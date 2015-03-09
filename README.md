## Notekeeper Open

Notekeeper Open is a Qt-based Evernote client for
Symbian and Meego-Harmattan.

This app was sold as [Notekeeper] in the Nokia Store
from 2012 till the closure of the store in 2015. This
is the complete source code for the app.

This repo also includes third-party code that is 
used in Notekeeper.

[Notekeeper]: http://notekeeperapp.com/

### Before building

The Evernote API keys and encryption keys used in Notekeeper are
not included in this repository in the interest of security.

To build a working copy of Notekeeper, you will need to:

  1. Obtain a fresh API key from Evernote
  2. Create a random key for encryption

Copy `notekeeper_config.h.template` in the repository as
`notekeeper_config.h` and populate the fields using the
instructions included in the template. After that, you 
can proceed to build Notekeeper.

### Building

Notekeeper should be built with Qt 4.7 or Qt 4.8.
It's recommended to build using Qt Creator.

The Symbian version is best built in Windows. The Meego-Harmattan
version can be built in Windows, Mac or Linux.

While seeing the project in Qt Creator, if you aren't able to
open the QML files for editing, try commenting out this line 
in the .pro file:

```
DEFINES += QML_IN_RESOURCE_FILE
```

### Design

The app uses Qt/QML for the UI and Qt/C++ for backend code
(storage, talking to Evernote, etc.).

Sharing of QML code between the Symbian and Meego-Harmattan
versions is acheived by using a base of common components
that in turn refer to the platform-specific components.

