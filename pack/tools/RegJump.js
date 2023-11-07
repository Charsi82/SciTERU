/*
Registry Jump
Version: 3.0
Author: mozers™ <http://html-applications.bitbucket.org>
------------------------------------------------
Open a new instance of the Registry Editor on the specified key.
The key can be input to the script in two ways:
  1. Through the command line argument
  2. Through the clipboard (The key is to be copied into a buffer before running the script)

Recognize entries of the any form:
  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control
  [HKLM\SYSTEM\CurrentControlSet\Control]
  HKLM\\SYSTEM\\CurrentControlSet\\Control
*/

var WshShell = new ActiveXObject("WScript.Shell");

var oArgs = WScript.Arguments;
var key = oArgs.length ? oArgs(0) : '';

if (!key) {
	try {
		key = (new ActiveXObject("htmlfile")).parentWindow.clipboardData.getData('Text');
	} catch(e) {}
}

if (key) {
	key = key.replace(/^HKLM\\/,'HKEY_LOCAL_MACHINE\\');
	key = key.replace(/^HKCU\\/,'HKEY_CURRENT_USER\\');
	key = key.replace(/^HKCR\\/,'HKEY_CLASSES_ROOT\\');
	key = key.replace(/^HKU\\/, 'HKEY_USERS\\');
	key = key.replace(/^HKCC\\/,'HKEY_CURRENT_CONFIG\\');
	key = key.replace(/\\\\/g,'\\');
	key = key.replace(/^\s*\[/,'').replace(/\]\s*$/,'');
}

if (/^HKEY_/i.test(key)) {
	try {
		WshShell.RegRead(key + '\\');
	} catch(e) {
		WshShell.Popup("Key\n" + key + "\nnot exist!", 3, "Registry Jump", 48);
	}

	var LastKey = 'HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\Lastkey';
	try {
		var computer = WshShell.RegRead(LastKey).replace(/\\.+$/, '') || 'Computer';
		WshShell.RegWrite (LastKey, computer + '\\' + key, 'REG_SZ');
	} catch(e){};
} else {
	WshShell.Popup("Registry key not defined!\nStart RegEdit default...", 2, "Registry Jump", 48);
}

WshShell.Run('regedit -m', 1);
