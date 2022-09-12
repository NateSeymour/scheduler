#include <iostream>
#include <toml++/toml.h>
#include <ostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <thread>
#include <cerrno>
#include <cstring>
#include <cmath>
#include <sys/resource.h>
#include "Agent.h"
#include "message/NysMqReceiver.h"

namespace fs = std::filesystem;
namespace chrono = std::chrono;

void Agent::UpdateDatabase()
{
    // Open / create database file
    fs::path database_path = this->mission.base / "nys.db";

    if(sqlite3_open(database_path.c_str(), &this->database) != SQLITE_OK)
    {
        throw std::runtime_error("Unable to open database file!");
    }

    // Get current version
    sqlite3_stmt *stmt_get_version;
    sqlite3_prepare_v2(this->database, "PRAGMA user_version;", -1, &stmt_get_version, nullptr);

    sqlite3_step(stmt_get_version);
    this->database_version = sqlite3_column_int(stmt_get_version, 0);

    sqlite3_finalize(stmt_get_version);

    // Load database schema info
    fs::path schema_directory = this->mission.database_resources;
    fs::path schema_info_file = schema_directory / "database.toml";

    toml::table schema_info = toml::parse_file(schema_info_file.c_str());

    long schema_latest = schema_info["database"]["latest"].as_integer()->get();

    if(this->database_version < schema_latest)
    {
        logger->Log("The database is out of date. Updating from version %i to %i...", this->database_version, schema_latest);
    }

    // Update database if it is not using the latest schema
    while(this->database_version < schema_latest)
    {
        logger->Log("Updating the database to version %i...", this->database_version + 1);

        // Get file path for the next schema file
        std::ostringstream next_version_file;
        next_version_file << "VERSION-" << this->database_version + 1 << ".sql";

        fs::path next_version_file_path = schema_directory / next_version_file.str();

        if(!fs::exists(next_version_file_path))
        {
            throw std::runtime_error("Missing schema file!");
        }

        // Open and read the file
        std::ifstream schema_file_in(next_version_file_path);
        std::string line;
        std::ostringstream schema_contents;
        if(schema_file_in.is_open())
        {
            while(std::getline(schema_file_in, line))
            {
                schema_contents << line << "\n";
            }
            schema_file_in.close();
        }
        else
        {
            throw std::runtime_error("Unable to open schema file!");
        }

        // Prepare and run SQL stmt_get_version
        if(sqlite3_exec(this->database, schema_contents.str().c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error("Error executing schema update file!");
        }

        logger->Log("Finished updating database.");

        this->database_version++;
    }

    // If we have surpassed the latest schema version, throw an error.
    if(this->database_version > schema_latest)
    {
        throw std::runtime_error("The database version has surpassed the latest schema version!");
    }
}

void Agent::LoadUnits()
{
    std::vector<std::filesystem::path> unit_paths;

    for(auto const& entry : fs::recursive_directory_iterator(this->mission.base / "schedule"))
    {
        if(entry.is_regular_file() && entry.path().extension() == ".unit")
        {
            logger->Log("Found: `%s`", entry.path().filename().c_str());
            unit_paths.push_back(entry.path());
        }
    }

    // Parse units
    for(auto const& unit_path : unit_paths) {
        auto unit = std::make_shared<Unit>();

        toml::table unit_table = toml::parse_file(unit_path.c_str());

        unit->path = unit_path;
        unit->name = unit_path.stem();

        // Exec
        if(!unit_table.at_path("unit.exec").is_string())
        {
            logger->Error("WARN Unit `%s` does not contain an `exec` command. Skipping...", unit->name.c_str());
            continue;
        }

        unit->exec = unit_table.at_path("unit.exec").as_string()->get();

        if(unit->exec == "")
        {
            logger->Error("WARN Unable to resolve the exec path `%s` for Unit `%s`. Skipping...", unit->exec.c_str(), unit->name.c_str());
            continue;
        }

        // Resolve the exec command to scripts if it is not in path
        if(!fs::exists(unit->exec))
        {
            if(fs::exists(this->mission.base / "scripts" / unit->exec))
            {
                unit->exec = this->mission.base / "scripts" / unit->exec;
            }
            else
            {
                logger->Error("WARN Unable to resolve the exec path `%s` for Unit `%s`. Skipping...", unit->exec.c_str(), unit->name.c_str());
                continue;
            }
        }

        // Ensure that the exec command is also executable
        if(access(unit->exec.c_str(), X_OK) != 0)
        {
            logger->Error("WARN Agent does not have permission to execute `%s` for Unit `%s`. Skipping...", unit->exec.c_str(), unit->name.c_str());
            continue;
        }

        // Trigger
        if(!unit_table.at_path("unit.trigger").is_string())
        {
            logger->Error("WARN Unit `%s` does not have a `trigger` defined. Skipping...", unit->name.c_str());
            continue;
        }

        unit->trigger = unit_table.at_path("unit.trigger").as_string()->get();

        // Args
        if(unit_table.at_path("unit.args").is_array())
        {
            auto toml_args = *unit_table.at_path("unit.args").as_array();

            for(auto&& arg : toml_args)
            {
                unit->arguments.push_back(arg.as_string()->get());
            }
        }

        // Description
        if(unit_table.at_path("unit.description").is_string())
        {
            unit->description = unit_table.at_path("unit.description").as_string()->get();
        }

        // Check database for last ran time
        sqlite3_stmt *last_run = nullptr;
        sqlite3_prepare_v2(this->database, "SELECT ScheduledTimeESEC, TimeExecutedESEC FROM MissionHistory WHERE UnitName = ? ORDER BY TimeExecutedESEC DESC;", -1, &last_run, nullptr);

        sqlite3_bind_text(last_run, 1, unit->name.c_str(), -1, SQLITE_STATIC);

        switch(sqlite3_step(last_run))
        {
            case SQLITE_ROW:
            {
                sqlite3_int64 scheduled_time_esecs = sqlite3_column_int64(last_run, 0);
                sqlite3_int64 time_executed_esecs = sqlite3_column_int64(last_run, 1);

                sqlite3_finalize(last_run);

                int64_t last_run_esecs = 0;

                switch(unit->scheduling_behavior)
                {
                    case SCHEDULING_BATCH:
                    {
                        last_run_esecs = time_executed_esecs;
                        break;
                    }

                    case SCHEDULING_STRICT:
                    {
                        last_run_esecs = scheduled_time_esecs;
                        break;
                    }
                }

                if(last_run_esecs != 0)
                {
                    unit->last_executed = chrono::sys_seconds(chrono::seconds(last_run_esecs));
                }
                else
                {
                    unit->last_executed = chrono::system_clock::now();
                }
                break;
            }

            case SQLITE_DONE:
            {
                unit->last_executed = chrono::system_clock::now();
                break;
            }

            default:
            {
                throw std::runtime_error("Database issues!");
            }
        }

        this->units.push_back(unit);
    }
}

int Agent::RunTask(const ScheduledTask& task)
{
    logger->Log("Starting unit `%s` (%s)...", task.unit->name.c_str(), task.unit->description.c_str());

    /*
     * Here we start two clocks. The sysclock is for logging and documentation purposes.
     * The steady clock is more reliable for measuring code execution time.
     */
    auto sysclock_start = chrono::system_clock::now();
    auto clock_start = chrono::steady_clock::now();

    pid_t child_pid = fork();

    if(child_pid > 0) // Parent
    {
        int stat_loc;
        struct rusage rusage{};

        wait4(child_pid, &stat_loc, 0, &rusage);

        auto clock_end = chrono::steady_clock::now();
        chrono::duration<double> time_difference = clock_end - clock_start;

        int exit_status = WEXITSTATUS(stat_loc);

        logger->Log("Unit `%s` completed execution in %ums with code %i.", task.unit->name.c_str(), (unsigned int)floor(time_difference.count() * 1000), exit_status);

        switch(task.unit->scheduling_behavior)
        {
            case SCHEDULING_BATCH:
            {
                task.unit->last_executed = chrono::system_clock::now();
                break;
            }

            case SCHEDULING_STRICT:
            {
                task.unit->last_executed = task.scheduled_time;
                break;
            }
        }

        sqlite3_stmt *update_db;
        const char * qstring = "INSERT INTO MissionHistory (UnitName, UnitTrigger, UnitDescription, ScheduledTimeESEC, TimeExecutedESEC, RunTimeMS, ExitCode) VALUES (?,?,?,?,?,?,?);";
        sqlite3_prepare_v2(this->database, qstring, -1, &update_db, nullptr);

        sqlite3_bind_text(update_db, 1, task.unit->name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(update_db, 2, task.unit->trigger.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(update_db, 3, task.unit->description.c_str(), -1, SQLITE_STATIC);

        sqlite3_int64 scheduled_time_esecs = chrono::duration_cast<chrono::seconds>(task.scheduled_time.time_since_epoch()).count();
        sqlite3_bind_int64(update_db, 4, scheduled_time_esecs);

        sqlite3_int64 time_executed_esecs = chrono::duration_cast<chrono::seconds>(sysclock_start.time_since_epoch()).count();
        sqlite3_bind_int64(update_db, 5, time_executed_esecs);

        sqlite3_int64 run_time_ms = chrono::duration_cast<chrono::milliseconds>(clock_end - clock_start).count();
        sqlite3_bind_int64(update_db, 6, run_time_ms);

        sqlite3_bind_int(update_db, 7, exit_status);

        sqlite3_step(update_db);

        sqlite3_finalize(update_db);

        return exit_status;
    }
    else // Child
    {
        // Redirect STDOUT and STDERR to log file
        fs::path unit_log_path = this->mission.base / "log" / (task.unit->name + ".log");

        logger->LogTag(task.unit->name.c_str(), "Logs for this unit can be found in %s.", unit_log_path.c_str());

        freopen(unit_log_path.c_str(), "a", stdout);
        freopen(unit_log_path.c_str(), "a", stderr);

        // Set working directory
        fs::path working_directory = this->mission.base / "workspace" / task.unit->name;
        fs::create_directories(working_directory);
        fs::current_path(working_directory);

        /*
         * Prepare arguments.
         *
         * Here, we need to allocate space for the path to the executable, the arguments and the null terminator.
         * *alloc without a matching free may look scary, but the memory needs to be valid for the entire runtime
         * of the child process and will be deallocated by the OS when the process finishes.
         *
         * *alloc is guaranteed to fill the memory space with 0's, so there is no need to explicitly set the null terminator.
         * See: `man malloc`
         */
        const char **argv = (const char **)calloc(task.unit->arguments.size() + 2, sizeof(char*));
        argv[0] = task.unit->exec.c_str();

        for(int i = 0; i < task.unit->arguments.size(); i++)
        {
            argv[i + 1] = task.unit->arguments[i].c_str();
        }

        // Launch Unit
        logger->LogTag(task.unit->name.c_str(), "Launching...");

        execv(task.unit->exec.c_str(), (char *const *)argv);

        logger->ErrorTag(task.unit->name.c_str(), "Unable to launch due to following error: `%s`", strerror(errno));

        exit(1);
    }
}

void Agent::LowPriorityRunner()
{
    Logger lplog("LPR", this->mission.base / "log" / "agent.log");

    lplog.Log("Low Priority Runner has started. %u task(s) in queue.", this->low_priority_queue.Count());

    NysMqReceiver receiver(this->mission.broadcaster);

    while(true)
    {
        auto message = receiver.Consume();
        switch(message.type)
        {
            case MESSAGE_SHUTDOWN:
                return;

            default:
                break;
        }

        if(this->low_priority_queue.Count() <= 0)
        {
            std::unique_lock lk(this->low_priority_queue.m_queue);
            this->low_priority_queue.cv_queue.wait(lk);

            continue;
        }

        auto next_task = this->low_priority_queue.ConsumeNext();

        auto time_until_next = next_task.scheduled_time - chrono::system_clock::now();
        auto time_in_minutes = chrono::duration_cast<chrono::duration<double, std::ratio<60>>>(time_until_next);
        double round_time_in_minutes = std::round(time_in_minutes.count() * 1000.0) / 1000.0;

        lplog.Log("Next task is Unit `%s`. It will run in %.1f minutes. There are %u task(s) in queue.", next_task.unit->name.c_str(), round_time_in_minutes, this->low_priority_queue.Count());

        /*
         * Here we grab a mutex lock and wait on the condition variable.
         * This allows us to wait until the trigger time of the next task
         * while simultaneously being able to react to changes in the queue.
         */
        {
            std::unique_lock lk(this->low_priority_queue.m_queue);
            this->low_priority_queue.cv_queue.wait_until(lk, next_task.scheduled_time);
        }

        // If the trigger is close enough, run the task. Else, replace it in the queue and re-loop.
        if(next_task.scheduled_time <= chrono::system_clock::now() + chrono::seconds(30))
        {
            this->RunTask(next_task);

            // Reschedule task
            ScheduledTask scheduled_task(this->trigger_parser.NextTrigger(next_task.unit), next_task.unit);
            this->low_priority_queue.Add(scheduled_task);
        }
        else
        {
            this->low_priority_queue.Add(next_task);
        }
    }
}

int Agent::Run()
{
    logger->Log("Agent initializing...");

    // Setup
    logger->Log("Updating database...");
    this->UpdateDatabase();

    logger->Log("Loading units...");
    this->LoadUnits();

    // Schedule units
    logger->Log("Scheduling units...");
    for(auto const& unit : this->units)
    {
        switch(unit->priority)
        {
            case PRIORITY_LOW:
            {
                ScheduledTask scheduled_task(this->trigger_parser.NextTrigger(unit), unit);
                this->low_priority_queue.Add(scheduled_task);
                break;
            }

            case PRIORITY_MEDIUM:
            case PRIORITY_HIGH:
            {
                logger->Error("WARN `PRIORITY_MEDIUM` and `PRIORITY_HIGH` priority scheduling are not yet implemented!");
                break;
            }
        }
    }

    // Start LowPriorityRunner
    logger->Log("Starting Low Priority Runner...");
    std::thread lp_thread(&Agent::LowPriorityRunner, this);

    // Message loop
    NysMqReceiver mq_receiver(this->mission.broadcaster);

    while(true)
    {
        NysMessage message = mq_receiver.ConsumeNext();

        // Inform LPQ about the new message
        this->low_priority_queue.Poke();

        switch(message.type)
        {
            case MESSAGE_SHUTDOWN:
            {
                logger->Log("Shutting down gracefully...");
                lp_thread.join();
                this->Cleanup();

                return 0;
            }

            case MESSAGE_NONE:
            {
                break;
            }
        }
    }
}

void Agent::Cleanup()
{
    if(this->database != nullptr)
    {
        sqlite3_close(this->database);
    }
}
