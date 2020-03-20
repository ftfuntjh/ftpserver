# FTPServer
FTPServer is a [FTP(file transfer protocol)](https://tools.ietf.org/html/rfc765) implement , based on [ACE(The Adaptive Communication Environment)](https://www.dre.vanderbilt.edu/~schmidt/ACE.html).
it based FTP Protocol [RFC 765]() and [RFC 959]() specification.

for more information about FTP Protocol.Please read the [wikepedia reference page](https://en.wikipedia.org/wiki/File_Transfer_Protocol)

# Install & Build
This project support Windows , Linux , Darwin platform. using [cmake](https://cmake.org/) build tool. 

## for linux user

### Require
- Install Cmake build tool.
- Install gcc/g++ build essential.

# Implement command
- USER
- PASS
- SIZE
- LIST
- PASV
- PWD
- TYPE
- CWD
- RETR
- STOR
- MKD
- SYST
- FEAT
- MDTM
- OPTS
- MLSD
- DELE
- QUIT
- RNFR
- RNTO
- SITE
