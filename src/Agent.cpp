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
#include "Agent.h"

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
    fs::path schema_directory = this->mission.binary.parent_path() / "database";
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

        this->units.push_back(unit);
    }
}

int Agent::RunUnit(std::shared_ptr<Unit> unit)
{
    logger->Log("Starting unit `%s` (%s)...", unit->name.c_str(), unit->description.c_str());

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

        logger->Log("Unit `%s` completed execution in %ums with code %i.", unit->name.c_str(), (unsigned int)floor(time_difference.count() * 1000), exit_status);

        return exit_status;
    }
    else // Child
    {
        // Redirect STDOUT and STDERR to log file
        fs::path unit_log_path = this->mission.base / "log" / (unit->name + ".log");

        logger->LogTag(unit->name.c_str(), "Logs for this unit can be found in %s.", unit_log_path.c_str());

        freopen(unit_log_path.c_str(), "a", stdout);
        freopen(unit_log_path.c_str(), "a", stderr);

        // Launch Unit
        logger->LogTag(unit->name.c_str(), "Launching...");

        char *const argv[] = {(char*)fs::current_path().c_str(), nullptr};
        execv(unit->exec.c_str(), argv);

        logger->ErrorTag(unit->name.c_str(), "Unable to launch due to following error: `%s`", strerror(errno));

        exit(1);
    }
}

void Agent::LowPriorityRunner()
{
    Logger lplog("LPR", this->mission.base / "log" / "agent.log");

    lplog.Log("Low Priority Runner has started. %u task(s) in queue.", this->low_priority_queue.Count());

    while(this->low_priority_queue.Count() > 0)
    {

        auto next_task = this->low_priority_queue.ConsumeNext();

        lplog.Log("Next task is Unit `%s`. There are %u task(s) in queue.", next_task.unit->name.c_str(), this->low_priority_queue.Count());

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
            this->RunUnit(next_task.unit);

            // TODO: Implement batching!
            ScheduledTask scheduled_task(next_task.unit->NextTrigger(next_task.scheduled_time), next_task.unit);
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
            case LOW:
            {
                // TODO: change to time loaded from DB
                ScheduledTask scheduled_task(unit->NextTrigger(chrono::system_clock::now()), unit);
                this->low_priority_queue.Add(scheduled_task);
                break;
            }

            case MEDIUM:
            case HIGH:
            {
                logger->Error("WARN `MEDIUM` and `HIGH` priority scheduling not yet implemented!");
                break;
            }
        }
    }

    // Start LowPriorityRunner
    logger->Log("Starting Low Priority Runner...");
    std::thread lp_thread(&Agent::LowPriorityRunner, this);
    lp_thread.join();

    // Cleanup
    this->Cleanup();

    return 0;
}

void Agent::Cleanup()
{
    if(this->database != nullptr)
    {
        sqlite3_close(this->database);
    }
}
