#!/usr/bin/env python3
# UpdateSizeInDocs.py
# Update the documented sizes of downloads to match the current release.
# Uses local build directory (../../../arc/upload<version>) on Neil's machine
# so must be modified for other installations.
# Implemented 2019 by Neil Hodgson neilh@scintilla.org
# Requires Python 3.6 or later

import pathlib

downloadHome = "https://www.scintilla.org/"

def ExtractFileName(s):
    url = s.split('"')[1]
    return url.rpartition('/')[2]

def FileSizeInMB(filePath):
    size = filePath.stat().st_size
    sizeInM = size / 1024 / 1024
    roundToNearest = round(sizeInM * 10) / 10
    return str(roundToNearest) + "M"

def FileSizesInDirectory(base):
    return {p.name : FileSizeInMB(p) for p in base.glob("*")}

def UpdateFileSizes(scriptsPath):
    sciteRoot = scriptsPath.parent
    scintillaRoot = sciteRoot.parent / "scintilla"
    lexillaRoot = sciteRoot.parent / "lexilla"
    releaseRoot = sciteRoot.parent.parent / "arc"

    uploadDocs = [
        sciteRoot / "doc" / "SciTEDownload.html",
        scintillaRoot / "doc" / "ScintillaDownload.html",
        lexillaRoot / "doc" / "LexillaDownload.html",
    ]

    version = (sciteRoot / "version.txt").read_text().strip()
    currentRelease = releaseRoot / ("upload" + version)
    fileSizes = FileSizesInDirectory(currentRelease)
    if not fileSizes:
        print("No files in", currentRelease)

    for docFileName in uploadDocs:
        outLines = ""
        original = docFileName.read_text()
        for line in original.splitlines(keepends=True):
            if downloadHome in line and '(' in line and ')' in line:
                fileName = ExtractFileName(line)
                if fileName in fileSizes:
                    pre, _bracket, rest = line.partition('(')
                    size, _rbracket, end = rest.partition(')')
                    if size != fileSizes[fileName]:
                        line = f"{pre}({fileSizes[fileName]}){end}"
                        print(f"{size} -> {fileSizes[fileName]} {fileName}")
            outLines += line
        if outLines != original:
            print("Updating", docFileName)
            docFileName.write_text(outLines)

if __name__=="__main__":
    UpdateFileSizes(pathlib.Path(__file__).resolve().parent)
