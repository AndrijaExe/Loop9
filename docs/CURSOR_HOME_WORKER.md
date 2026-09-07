# Cursor home worker (Windows + Unreal)

Personal runbook: leave the Windows Unreal machine on at home, then prompt it
from a laptop, phone, Slack, or [cursor.com/agents](https://cursor.com/agents).

Official docs:

- [My Machines](https://cursor.com/docs/cloud-agent/self-hosted/my-machines)
- [Computer use](https://cursor.com/docs/cloud-agent/self-hosted/computer-use)
- [CLI install](https://cursor.com/docs/cli/installation)

## What you get

The agent loop (thinking) runs in Cursor's cloud. File edits, the terminal,
local MCP, and (if supported) mouse/keyboard run **on this Windows PC**.

Nothing inbound is required. The worker opens outbound HTTPS to Cursor.

| Surface | Use this machine |
|---|---|
| [cursor.com/agents](https://cursor.com/agents) | Pick `kuca-unreal` in the environment dropdown |
| Slack | `@Cursor worker=kuca-unreal ...` |
| GitHub | `@cursoragent worker=kuca-unreal ...` |
| Linear | put `worker=kuca-unreal` in the issue body |

If you omit `worker=kuca-unreal`, Cursor uses a **cloud VM**. That VM does not
have Unreal, marketplace content, or this checkout.

## Honest limit: computer-use on Windows

You asked for full desktop control (`--computer-use`). Cursor's current docs
enable that flag on **macOS and Linux only**. `--share-desktop` is **Linux
only**.

On this Windows box, still start the worker with `--computer-use`:

1. If the CLI accepts it, you get clicks, typing, and screenshots on the
   signed-in desktop. That is the full-control path.
2. If the CLI rejects the flag or screenshots never appear, leave the rest of
   this guide as-is. You still have the useful Unreal path: **repo + terminal +
   Unreal MCP** on the machine that already has UE 5.8.

Do **not** try to "fix" this with WSL computer-use. A WSL desktop cannot click
the native Unreal Editor.

Do **not** port-forward Unreal Remote Control or `127.0.0.1:8000` to the
internet. The worker already reaches localhost.

## One-time setup (at home, PowerShell)

Edit these two paths if your disks differ, then keep them for the rest of the
guide.

```powershell
$GameRepo    = "D:\UE Course\Loop 9 AI\Loop9 5.8"
$BackendRepo = "C:\Users\andri\OneDrive\Desktop\Loop9Backend\my_project_directory"
$UeEditor    = "D:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$WorkerName  = "kuca-unreal"
```

### 1. Paid Cursor account

Cloud Agents / My Machines need a **paid** Cursor plan, signed into the **same**
account you use on the laptop/phone.

### 2. Keep the PC awake and logged in

The worker needs a live Windows session.

1. Settings → System → Power: plugged-in, **Never** sleep, **Never** hibernate.
2. Settings → System → Screen: screen can turn off; do not sleep.
3. Settings → System → Power → Additional power settings → Change plan settings
   → Change advanced power settings → Sleep → Hibernate after = **Never**.
4. Stay logged in. Locking the screen is fine for MCP/code; computer-use needs
   the desktop session alive (not a fast-user-switch empty session).
5. Optional: BIOS/Windows "Wake on LAN" if you want to wake it from another
   device. The worker itself does not wake a sleeping PC.

### 3. Install Cursor CLI

PowerShell:

```powershell
irm 'https://cursor.com/install?win32=true' | iex
```

Close the window, open a **new** PowerShell, then:

```powershell
agent --version
agent update
```

If `agent` is not found, add `%USERPROFILE%\.local\bin` to your user PATH and
open a new terminal.

### 4. Sign in

```powershell
agent login
```

Use the same Cursor account as on the laptop. Browser login is enough. A
personal API key from Cursor Dashboard → API Keys also works
(`agent worker start --api-key "..."`). Do not commit that key.

### 5. Confirm the git remotes

The worker routes by **git remote**, not by folder name. Slack/GitHub
`worker=kuca-unreal` only lands here if this checkout's `origin` is the repo
you mentioned.

```powershell
cd $GameRepo
git remote -v
git status
```

Expected game remote: `git@github.com:AndrijaExe/Loop9.git` (or the HTTPS
equivalent). If you also register the backend, check that checkout too.

### 6. Preflight

```powershell
cd $GameRepo
agent worker debug
```

Fix auth / PATH / outbound HTTPS before the next step. Required outbound hosts:

- `api2.cursor.sh`
- `api2direct.cursor.sh`
- `downloads.cursor.com`
- `cloud-agent-artifacts.s3.us-east-1.amazonaws.com`

No inbound ports, no router port-forward, no public Unreal port.

### 7. Start the worker (leave this window open the first time)

From the game repo, with flags **before** `start`:

```powershell
cd $GameRepo

agent worker `
  --name $WorkerName `
  --computer-use `
  --worker-dir $GameRepo `
  --worker-dir $BackendRepo `
  start
```

Drop `--worker-dir $BackendRepo` if that folder is not on this PC.

Leave the process running. The machine should appear at
[cursor.com/agents](https://cursor.com/agents) in the environment dropdown.

If `--computer-use` errors on Windows, rerun the same command without that
flag. Code + terminal + MCP still work.

### 8. Open Unreal with MCP

Epic's built-in plugin listens on `127.0.0.1:8000/mcp`. Auto-start is **off**
for cooks (see `Config/DefaultEditorPerProjectUserSettings.ini`), so start it
on the interactive editor:

```powershell
& $UeEditor "$GameRepo\Loop9.uproject" -ModelContextProtocolStartServer
```

Or, with the editor already open: `ModelContextProtocol.StartServer` in the
Output Log / console.

Confirm in the Unreal Output Log that MCP started on port **8000**.

This HTTP endpoint is **localhost only**. Cursor Cloud Agents treat HTTP MCP as
a **cloud-side** connection, so they cannot reach `127.0.0.1:8000` on this PC.
Add a **stdio** MCP that runs **on the worker** and talks to localhost.

In [cursor.com/agents](https://cursor.com/agents) → MCP dropdown → add a
personal **stdio** (command) server, not an HTTP URL. Example if Node.js is on
PATH (adjust if you already use another stdio Unreal bridge):

```text
command: npx
args:    -y mcp-proxy http://127.0.0.1:8000/mcp
```

Cursor Cloud Agents do **not** support `mcp-remote`. Stdio must run on this
machine so it can see Unreal.

If the proxy package fails, any small stdio MCP that forwards to
`http://127.0.0.1:8000/mcp` is fine. The important part is **stdio on the
worker**, not an HTTP URL in the cloud dashboard.

### 9. First remote test (from the laptop or phone)

1. Open [cursor.com/agents](https://cursor.com/agents).
2. Select environment `kuca-unreal`.
3. Prompt: `What is the current git branch and last commit in this checkout?`
4. Prompt: `List Unreal MCP tools, then take a viewport screenshot of FullOfficeMap.`
5. If `--computer-use` started: `Open Notepad, type LOOP9 WORKER OK, take a screenshot.`

You want file/git answers from **this** Windows checkout, MCP tools from the
live editor, and (only if computer-use is live) a desktop screenshot.

## Every time you leave the house

Checklist:

1. Windows user logged in, PC plugged in, sleep/hibernate off.
2. `git pull` on the game repo (and backend, if registered).
3. Unreal Editor open on `Loop9.uproject` with MCP on port 8000.
4. Worker process running (window, or the Scheduled Task below).
5. From elsewhere, pick `kuca-unreal` or pass `worker=kuca-unreal`.

Marketplace packs stay on this disk (~19 GB local project). The cloud VM does
not have them. That is the point of the home worker.

## Autostart after reboot

Use Task Scheduler so you do not have to remember the command. The task must
run **only when you are logged on** (interactive session). "Run whether user
is logged on or not" will not drive Unreal or computer-use.

1. Save this as `%USERPROFILE%\start-kuca-unreal-worker.ps1` and fix the paths:

```powershell
$ErrorActionPreference = "Stop"
$GameRepo    = "D:\UE Course\Loop 9 AI\Loop9 5.8"
$BackendRepo = "C:\Users\andri\OneDrive\Desktop\Loop9Backend\my_project_directory"
$UeEditor    = "D:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$WorkerName  = "kuca-unreal"

# Uncomment if you want the editor to come back after reboot:
# if (-not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) {
#   Start-Process $UeEditor -ArgumentList "`"$GameRepo\Loop9.uproject`" -ModelContextProtocolStartServer"
# }

$agent = Get-Command agent -ErrorAction SilentlyContinue
if (-not $agent) {
  $agentPath = Join-Path $env:USERPROFILE ".local\bin\agent.exe"
  if (Test-Path $agentPath) { $agent = Get-Item $agentPath }
}
if (-not $agent) { throw "Cursor CLI 'agent' not on PATH" }

$workerArgs = @(
  "worker",
  "--name", $WorkerName,
  "--computer-use",
  "--worker-dir", $GameRepo
)
if (Test-Path $BackendRepo) {
  $workerArgs += @("--worker-dir", $BackendRepo)
}
$workerArgs += "start"

Set-Location $GameRepo
& $agent.Source @workerArgs
```

2. Task Scheduler → Create Task (not Basic Task):

   - General: name `Loop9 Cursor home worker`; run only when user is logged on;
     run with highest privileges only if Unreal/MCP needs it (usually no).
   - Triggers: At log on (your Windows user).
   - Actions: Start a program
     - Program: `powershell.exe`
     - Arguments:
       `-NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\start-kuca-unreal-worker.ps1`
   - Conditions: uncheck "Start the task only if the computer is on AC power"
     if you want it on battery; prefer AC.
   - Settings: "If the task fails, restart every 1 minute" (3 times is enough).

3. Reboot once, confirm [cursor.com/agents](https://cursor.com/agents) still
   lists `kuca-unreal`.

Unreal after reboot: either uncomment the `Start-Process` block, or open the
editor yourself before you leave. MCP is the editor control path; a worker
without Unreal can still edit C++/docs but cannot touch live assets.

## How to prompt from elsewhere

Good first line in every remote chat:

```text
Run on worker=kuca-unreal (Windows home Unreal checkout).
Do not use a Cursor cloud VM.
Use Unreal MCP if the editor is up. Do not expose port 8000.
```

Examples:

- `Pull main, rebuild Loop9Editor, then tell me if FullOfficeMap compiles.`
- `Use Unreal MCP: open FullOfficeMap and list the decoy observation zone volumes.`
- `Follow docs/HOME_EDITOR_TIMELINE_AND_ARCHIVE.md. C++ is done; edit the existing UMG assets only.`

For Slack/GitHub, the `worker=` token is what binds the run to this PC. The
machine name must match `--name`, the machine must belong to your Cursor
user, and the worker-dir git remote must be that repo.

## Security

- The agent can edit the repo, run shells, and (if computer-use works) click
  whatever is on the desktop. Do not leave banking, Steam Guard, or password
  managers unlocked on that screen.
- Screenshots and tool output leave the machine over the worker's HTTPS
  connection. Privacy Mode still applies if you have it on.
- Keep Unreal MCP and Remote Control on **127.0.0.1**. Never bind `0.0.0.0`
  or forward those ports on the router.
- Do not put API keys in the Scheduled Task arguments. Use `agent login` once
  interactively, or a user API key in a file you do not commit.

## Troubleshooting

| Symptom | Check |
|---|---|
| Machine missing in the dropdown | Worker process still running? Same Cursor account? `agent worker debug` |
| Task runs in the cloud VM | You forgot `worker=kuca-unreal` / environment dropdown |
| `worker=kuca-unreal` rejected for another repo | Start the worker with `--worker-dir` for that checkout, or pass the matching remote |
| Unreal MCP tools missing | Editor up? `-ModelContextProtocolStartServer`? Port 8000? MCP added as **stdio** on the worker, not HTTP in the cloud |
| `--computer-use` fails | Expected on current Windows CLI. Drop the flag. MCP + terminal remain |
| Worker dies after lock/sleep | Disable sleep/hibernate; keep the user session; use the logon task |
| `agent` not found in the task | Use the full path to `agent.exe` in the script |
| Git auth fails from the agent | Windows user already has `git` + SSH agent / Git Credential Manager for `origin` |

## Related Loop 9 docs

- [SETUP_AND_DEVELOPMENT.md](SETUP_AND_DEVELOPMENT.md)
- [HOME_EDITOR_TIMELINE_AND_ARCHIVE.md](HOME_EDITOR_TIMELINE_AND_ARCHIVE.md)
- [UNREAL_MCP_ENDING_SCENES_HANDOFF.md](UNREAL_MCP_ENDING_SCENES_HANDOFF.md)
- [CONTENT_AUTHORING.md](CONTENT_AUTHORING.md)
