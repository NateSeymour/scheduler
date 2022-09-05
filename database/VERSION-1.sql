PRAGMA user_version = 1;

CREATE TABLE MissionHistory (
    UnitName TEXT,
    UnitHash TEXT,
    UnitTrigger TEXT,
    UnitDescription TEXT,
    ScheduledTimeESEC INT,
    TimeExecutedESEC INT,
    RunTimeMS INT,
    ExitCode INT
);