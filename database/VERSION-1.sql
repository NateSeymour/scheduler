PRAGMA user_version = 1;

CREATE TABLE Units (
    Name TEXT NOT NULL,
    Hash TEXT NOT NULL
);

CREATE TABLE MissionHistory (
    UnitId integer NOT NULL,
    TimeExecuted integer NOT NULL,
    ExitCode integer NOT NULL
);