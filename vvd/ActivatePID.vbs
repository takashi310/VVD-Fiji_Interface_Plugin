dim  objArguments, pid
Set WshShell = CreateObject("WScript.Shell")
Set objArguments = Wscript.Arguments
pid = objArguments(0)
For index = 1 To 2 Step 1
WScript.Sleep 50
WshShell.AppActivate pid
Next