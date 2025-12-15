# Set thread scheduling policy to "Prefer performant processors" (2)

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "This script requires administrator privileges." -ForegroundColor Red
    Write-Host "Attempting to restart with administrator privileges..." -ForegroundColor Yellow

    try {
        Start-Process powershell.exe -ArgumentList "-ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
        exit
    } catch {
        Write-Host ""
        Write-Host "ERROR: Failed to elevate to administrator." -ForegroundColor Red
        Write-Host "Please right-click this script and select 'Run as Administrator'" -ForegroundColor Yellow
        Write-Host ""
        Read-Host "Press Enter to exit"
        exit 1
    }
}

$subgroup = "54533251-82be-4824-96c1-47b60b740d00"
$setting = "bae08b81-2d5e-4688-ad6a-13243356654b"

Write-Host "Setting thread scheduling policy to: Prefer performant processors (2)" -ForegroundColor Green

# Unhide the setting first
powercfg -attributes $subgroup $setting -ATTRIB_HIDE

# Set the policy
powercfg /setacvalueindex SCHEME_CURRENT $subgroup $setting 2
powercfg /setdcvalueindex SCHEME_CURRENT $subgroup $setting 2
powercfg /setactive SCHEME_CURRENT

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully set policy to: Prefer performant processors" -ForegroundColor Green
} else {
    Write-Host "ERROR: Failed to set policy. Make sure you have a hybrid CPU." -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to exit"
