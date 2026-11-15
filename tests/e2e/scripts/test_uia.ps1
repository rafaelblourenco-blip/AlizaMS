[Reflection.Assembly]::LoadWithPartialName('UIAutomationClient') | Out-Null
[Reflection.Assembly]::LoadWithPartialName('UIAutomationTypes') | Out-Null
$p = Start-Process 'C:\Projeto\Dicom Viewer\aliza-radiant-windows-x64-Build-2\aliza-radiant-windows-x64\alizams.exe' -ArgumentList '-nogl' -PassThru
Start-Sleep -Seconds 2
$hwnd = (Get-Process -Id $p.Id).MainWindowHandle
Write-Output "MainWindowHandle: $hwnd"
if ($hwnd -ne 0) {
    $elem = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$hwnd)
    Write-Output "Elem Name: $($elem.Current.Name)"
    Write-Output "Elem ControlType: $($elem.Current.ControlType.ProgrammaticName)"
    $children = $elem.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
    Write-Output "Child count: $($children.Count)"
    for ($i = 0; $i -lt [Math]::Min(25, $children.Count); $i++) {
        $c = $children[$i]
        Write-Output "  $($c.Current.ControlType.ProgrammaticName): '$($c.Current.Name)' | '$($c.Current.AutomationId)'"
    }
}
Stop-Process -Id $p.Id -Force
