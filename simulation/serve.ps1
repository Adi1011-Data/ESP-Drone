$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$python = $null
foreach ($name in @("python", "py")) {
  if (Get-Command $name -ErrorAction SilentlyContinue) {
    $python = $name
    break
  }
}
if (-not $python) {
  Write-Error "Python was not found. Install Python, or open the HTML files from a local server."
  exit 1
}

Write-Host "ESP-Drone tools:    http://127.0.0.1:8765/"
Write-Host "Attitude viewer:  http://127.0.0.1:8765/viewer/"
Write-Host "PID tuner:        http://127.0.0.1:8765/pid/"
Write-Host "Leave this window open. Close it to stop the server."
Start-Process powershell -ArgumentList "-NoProfile","-WindowStyle","Hidden","-Command","Start-Sleep -Seconds 1; Start-Process 'http://127.0.0.1:8765/'"
& $python -m http.server 8765 --bind 127.0.0.1 --directory $root
