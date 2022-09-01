PRAGMA user_version = 1;

CREATE TABLE Units (
    Name TEXT NOT NULL,
    Hash TEXT NOT NULL
);

CREATE TABLE MissionHistory (
    UnitId INTEGER NOT NULL,
    TimeExecuted INTEGER NOT NULL,
    ExitCode INTEGER NOT NULL
);