---
title: MNE CPP on Neuromag Linux
parent: Learn
nav_order: 4
---
# MNE CPP on Neuromag Linux

#### Warning: You should know what you are doing and be familiar with the Neuromag system!

## Setup mne_rt_server

The following steps are to be performed on the Neuromag PC.


### Qt 5

```
./configure -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-glib -no-gui -no-widgets -no-opengl -no-openssl -opensource -nomake examples -prefix /home/neuromag/RT_MNE_CPP/Qt/Qt5.3.0
make
```

It's likely that following error occurs: Futex Error (RHEL 5.1 with gcc 4.1.2 compiler)

You can fix this error by replacing following line in src/corelib/thread/qmutex_linux.cpp

```
# include <linux/futex.h>
```

with

```
# define FUTEX_WAIT 0
# define FUTEX_WAKE 1
```

You can ignore further Qt5 build errors. Since only qmake and the most important libraries are needed. Finish the installation with:

```
make install
```

### MNE-cpp

#### Download MNE-cpp

Navigate to the mne-cpp repository and run:

```
qmake mne-cpp.pro MNECPP_CONFIG+=minimalVersion MNECPP_CONFIG+=noExamples MNECPP_CONFIG+=noTests
make
```

#### Edit .bashrc

After you compiled mne-cpp succesfully edit your .bashrc to make the shared libraries globally available:

```
#
# Qt
#
export QT_HOME=/home/neuromag/RT_MNE_CPP/Qt/Qt5.3.0
if [[ $QT_HOME && ${QT_HOME-_} ]]; then
    PATH=$PATH:$QT_HOME/bin
    if [[ $LD_LIBRARY_PATH && ${LD_LIBRARY_PATH-_} ]]; then
       LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_HOME/lib
    else
       export LD_LIBRARY_PATH=$QT_HOME/lib
    fi
fi

#
#  MNE-CPP
#
export MNE_CPP=/home/neuromag/RT_MNE_CPP/Git_Repos/mne-cpp
if [[ $MNE_CPP && ${MNE_CPP-_} ]]; then
    PATH=$PATH:$MNE_CPP/bin
    if [[ $LD_LIBRARY_PATH && ${LD_LIBRARY_PATH-_} ]]; then
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MNE_CPP/lib
    else
        export LD_LIBRARY_PATH=$MNE_CPP/lib
    fi
fi
```

#### Firewall Configuration

Open port 4217 & 4218 and add allowed clients by editing /etc/sysconfig/iptables. Insert the following:

```
-A RH-Firewall-1-INPUT -m state --state NEW -m tcp -p tcp --dport 4217 -j ACCEPT
-A RH-Firewall-1-INPUT -m state --state NEW -m tcp -p tcp --dport 4218 -j ACCEPT
-A RH-Firewall-1-INPUT -s ip.of.your.client -m tcp -p tcp --dport 4217 -j ACCEPT
-A RH-Firewall-1-INPUT -s ip.of.your.client -m tcp -p tcp --dport 4218 -j ACCEPT
```

## Connect the Neuromag System with MNE Scan

The following steps are to be performed both on the Neuromag and Client PC.

### Step by Step Guide

1. Setup Neuromag PC:
    1. Open console, type

        ```
        mne_rt_server
        ```

        and hit enter (use this console to wathc the mne_rt_server work and receive commands from the client)

    2. Open second console, type

        ```
        telnet <ip.of.megacq> collector
        ```
        (use this console to restart and change variables of the MEG system).

            1. When asked for password type in "pass <your password>" and hit enter.
            2. Type in

                ```
                STAT
                ```
                to view the current status of the MEG machine. This should give you a message saying that the collectors are setup. If not get help to set the collectors up.
            3. Type in

                ```
                VARS
                ```

                and note down the values of maxBuflen and sFreq (these values get overwritten by MNE Scan). This is IMPORTANT!
    3. Find out IP adress of the Neuromag PC (this is NOT the IP shown by the mne_rt_server in the console window) and note it down somewhere.
    4. Check if ports 4217 and 4218 are open and if the client is allowed to connect. If not see the Firewall Configuration steps above.
2. Setup MNE-CPP PC:
    1. Try to ping the Neuromag PC from the MNE-CPP PC
            1. Open cmd console
            2. Type in

                ```
                ping <Neuromag PC IP address>
                ```

                (this is the IP address you noted down above).

    2. If pinging was successful start MNE Scan and drag the Neuomag plugin onto the plugin stage.
    3. Change to the Connection tab. There you should find the connected satuts. If not you made something wrong!

## Hints

### Use Cases

* MNE Scan crashed:

    1. Close down MNE Scan
    2. Switch to the Neuromag PC
    3. Hit Ctrl+C in the mne_rt_server console
    4. Restart the mne_rt_server by typing

      ```
      mne_rt_server
      ```

    5. Type

      ```
      STOP
      ```

      into the Collector console to stop the measurement system
      You are all set and ready for antoher try!

* Change the blocksizes of the incoming data:

    * Changing the block size is only possible by recompiling the mne_rt_server with a different initialized value in /connectors/neuromag/neuromag.cpp

### The collectors

You can use telnet to monitor the interaction of _mne_rt_server_ with the neuromag machine: Therefore connect to the collectors running on the real-time computers:

```
telnet megacq collector
```

or

```
telnet ip.of.megacq 11122
```

Authorize your self:

```
PASS ***
```

Check the status by typing (They should be setup but not measuring. If this is not the case run a simple dummy project using the neuromag acqusition software, afterwards they will be setup.):

```
STAT
```

Check the values of the variables by typing:

```
VARS
```

Save the variables in a text file and make sure that they are the same after you have finished (your colleagues will appreciate this)

You can change variable names by typing

```
VARA ``<var> = <value>``

```

Switch on the monitoring to follow all interactions with the real-time computers:

```
MONI ON
```

You can start the acqusition manually with:

```
MEAS
```

And stopping it again with:

```
STOP
```

All available commands you'll find by typing:

```
HELP
```

You can leave the collectors by typing:

```
QUIT
```

### Neuromag Acqusition

It's recommended to start the Neuromag Acquisition program. The software will be remotely controlled by MNE Scan since the collectors mirror the instructions received. You can use the Neuromag data display to check your MNE Scan measurements.
