# Not Your Scheduler

_NOTE: This project is in development and not stable. Do not use in production._

- Run `generate_cool_readme.js` `now`.
- Run `backup_repo.sh` `every 7 days`.

## Why shouldn't I use `cron` instead?

Don't get me wrong, `cron` is a great tool. But, Not Your Scheduler will save you from having migraines if you find yourself frequently asking the following questions:

### 1. Did my job actually run?

Any jobs that didn't run on schedule are automatically batched and run at the next opportunity by `nysd`. This means no more missed jobs due to system load or downtime.

NYS stores run information and statistics in a database in `~/.nys/nys.db`. You can use `nysctl` to get information about how often jobs fail, when they were last run, their current state and PID, etc. Or, you can load the file into your favorite databasing program and generate your own statistics and dashboards.

### 2. Where the hell are my logs?

Each `Unit` (job) has its logs stored in its own logfile in `~/.nys/log/${Unit}.log` by default. It will include the entirety of the program's `stdout` and `stderr` output. 

### 3. Was it hour minute day, or minute day year, or...?

Even if you're experienced in working with `cron`, `* 2 * * *` will never be very *readable*.

NYS uses `.toml` files to store its job configurations and human-readable syntax for job scheduling.

```toml
# ~/.nys/schedule/Backup.unit
[unit]

exec = "backup.js"
trigger = "every 7 days"
```

It truly couldn't be simpler or more readable. 

### 4. How do I know if my jobs failed?

NYS has the ability to notify you or run specific `Unit`s when a job fails. This way, you can have peace of mind that everything is running smoothly.

```toml
# ~/.nys/schedule/FailureNotification.unit
[unit]

exec = "notify_failure.js"
trigger = "on job_failure"
```

## Installation

NYS is installed using `brew`.

```shell
brew install nateseymour/nys/scheduler
```
