// This part of file from Notepad++ project
// ..\notepad-plus-plus\PowerEditor\src\WinControls\AnsiCharPanel\asciiListView.cpp
// Copyright (C)2021 Don HO <don.h@free.fr>

#include "string"

std::string getAscii(unsigned char value)
{
	switch (value)
	{
	case 0:
		return ("NULL");
	case 1:
		return ("SOH");
	case 2:
		return ("STX");
	case 3:
		return ("ETX");
	case 4:
		return ("EOT");
	case 5:
		return ("ENQ");
	case 6:
		return ("ACK");
	case 7:
		return ("BEL");
	case 8:
		return ("BS");
	case 9:
		return ("TAB");
	case 10:
		return ("LF");
	case 11:
		return ("VT");
	case 12:
		return ("FF");
	case 13:
		return ("CR");
	case 14:
		return ("SO");
	case 15:
		return ("SI");
	case 16:
		return ("DLE");
	case 17:
		return ("DC1");
	case 18:
		return ("DC2");
	case 19:
		return ("DC3");
	case 20:
		return ("DC4");
	case 21:
		return ("NAK");
	case 22:
		return ("SYN");
	case 23:
		return ("ETB");
	case 24:
		return ("CAN");
	case 25:
		return ("EM");
	case 26:
		return ("SUB");
	case 27:
		return ("ESC");
	case 28:
		return ("FS");
	case 29:
		return ("GS");
	case 30:
		return ("RS");
	case 31:
		return ("US");
	case 32:
		return ("Space");
	case 127:
		return ("DEL");
	default:
	{
		//char ascii[2]{value, '\0'};
		//return std::string(ascii);
		return std::string({ (char)value, '\0' });
	}

	}
}

std::string getHtmlName(unsigned char value)
{
	switch (value)
	{
	case 34:
		return ("&quot;");
	case 38:
		return ("&amp;");
	case 60:
		return ("&lt;");
	case 62:
		return ("&gt;");
	case 128:
		return ("&euro;");
	case 160:
		return ("&nbsp;");
	case 161:
		return ("&iexcl;");
	case 162:
		return ("&cent;");
	case 163:
		return ("&pound;");
	case 164:
		return ("&curren;");
	case 165:
		return ("&yen;");
	case 166:
		return ("&brvbar;");
	case 167:
		return ("&sect;");
	case 168:
		return ("&uml;");
	case 169:
		return ("&copy;");
	case 170:
		return ("&ordf;");
	case 171:
		return ("&laquo;");
	case 172:
		return ("&not;");
	case 173:
		return ("&shy;");
	case 174:
		return ("&reg;");
	case 175:
		return ("&macr;");
	case 176:
		return ("&deg;");
	case 177:
		return ("&plusmn;");
	case 178:
		return ("&sup2;");
	case 179:
		return ("&sup3;");
	case 180:
		return ("&acute;");
	case 181:
		return ("&micro;");
	case 182:
		return ("&para;");
	case 183:
		return ("&middot;");
	case 184:
		return ("&cedil;");
	case 185:
		return ("&sup1;");
	case 186:
		return ("&ordm;");
	case 187:
		return ("&raquo;");
	case 188:
		return ("&frac14;");
	case 189:
		return ("&frac12;");
	case 190:
		return ("&frac34;");
	case 191:
		return ("&iquest;");
	case 192:
		return ("&Agrave;");
	case 193:
		return ("&Aacute;");
	case 194:
		return ("&Acirc;");
	case 195:
		return ("&Atilde;");
	case 196:
		return ("&Auml;");
	case 197:
		return ("&Aring;");
	case 198:
		return ("&AElig;");
	case 199:
		return ("&Ccedil;");
	case 200:
		return ("&Egrave;");
	case 201:
		return ("&Eacute;");
	case 202:
		return ("&Ecirc;");
	case 203:
		return ("&Euml;");
	case 204:
		return ("&Igrave;");
	case 205:
		return ("&Iacute;");
	case 206:
		return ("&Icirc;");
	case 207:
		return ("&Iuml;");
	case 208:
		return ("&ETH;");
	case 209:
		return ("&Ntilde;");
	case 210:
		return ("&Ograve;");
	case 211:
		return ("&Oacute;");
	case 212:
		return ("&Ocirc;");
	case 213:
		return ("&Otilde;");
	case 214:
		return ("&Ouml;");
	case 215:
		return ("&times;");
	case 216:
		return ("&Oslash;");
	case 217:
		return ("&Ugrave;");
	case 218:
		return ("&Uacute;");
	case 219:
		return ("&Ucirc;");
	case 220:
		return ("&Uuml;");
	case 221:
		return ("&Yacute;");
	case 222:
		return ("&THORN;");
	case 223:
		return ("&szlig;");
	case 224:
		return ("&agrave;");
	case 225:
		return ("&aacute;");
	case 226:
		return ("&acirc;");
	case 227:
		return ("&atilde;");
	case 228:
		return ("&auml;");
	case 229:
		return ("&aring;");
	case 230:
		return ("&aelig;");
	case 231:
		return ("&ccedil;");
	case 232:
		return ("&egrave;");
	case 233:
		return ("&eacute;");
	case 234:
		return ("&ecirc;");
	case 235:
		return ("&euml;");
	case 236:
		return ("&igrave;");
	case 237:
		return ("&iacute;");
	case 238:
		return ("&icirc;");
	case 239:
		return ("&iuml;");
	case 240:
		return ("&eth;");
	case 241:
		return ("&ntilde;");
	case 242:
		return ("&ograve;");
	case 243:
		return ("&oacute;");
	case 244:
		return ("&ocirc;");
	case 245:
		return ("&otilde;");
	case 246:
		return ("&ouml;");
	case 247:
		return ("&divide;");
	case 248:
		return ("&oslash;");
	case 249:
		return ("&ugrave;");
	case 250:
		return ("&uacute;");
	case 251:
		return ("&ucirc;");
	case 252:
		return ("&uuml;");
	case 253:
		return ("&yacute;");
	case 254:
		return ("&thorn;");
	case 255:
		return ("&yuml;");
	default:
	{
		return ("");
	}

	}
}

int getHtmlNumber(unsigned char value)
{
	switch (value)
	{
	case 128:
		return 8364;
	case 130:
		return 8218;
	case 131:
		return 402;
	case 132:
		return 8222;
	case 133:
		return 8230;
	case 134:
		return 8224;
	case 135:
		return 8225;
	case 137:
		return 8240;
	case 138:
		return 352;
	case 140:
		return 338;
	case 145:
		return 8216;
	case 146:
		return 8217;
	case 147:
		return 8220;
	case 148:
		return 8221;
	case 149:
		return 8226;
	case 150:
		return 8211;
	case 151:
		return 8212;
	case 153:
		return 8482;
	case 154:
		return 353;
	case 156:
		return 339;
	case 159:
		return 376;
	default:
	{
		return -1;
	}

	}
}