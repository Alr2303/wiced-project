
Introduction
------------
As part of the BIG Mesh Ligting solution, there are 2 apps namely MeshService and MeshLighting:

    MeshService.apk : is an Andriod service , which manages all the network layer information related to mesh network.
    This app manages functionality such as creating a network , adding devices and provides data pipes to talk to mesh devices

    MeshLighting.apk : is an Android ligthing app which runs on top of MeshService. This app allows users to create a Lighting Network, add
    lights and control them for "inside home" and "outside home" usecase scenarios.


Installation and Usage
----------------------
1>  Install MeshService.apk and MeshLighting.apk on an Android phone.
    Grant permission when MeshLighting app opens for first time.
    Add all permissions to MeshService in settings->apps->meshservice->Permissions and reboot the phone
    (The preferred version would be Android 7.0)
2>  Install the wiced appplication mesh_onoff_bulb or mesh_dimmable_bulb in 20735-B0_Bluetooth from WICED Studio 5.0
    (app is located at \20735-B0_Bluetooth\apps\mesh_onoff_bulb)
3>  Build and run mesh_network demo application located under bluetooth_internet_gateway on WICED SDK
4>  When WICED BIG mesh powers up, the device will connect to a configured WiFi AP, and is ready to be provisioned
5>  Using the Android Ligthing App, create a lighting network and start adding Lights to the network
    a>  Create a room and add lights to the room
    b>  The application wraps provisioning and configuration as part of adding light 
        Once the provisioning is complete a "Provision Complete" Toast appears.
        During Configuration, application performs the following
           getConfiguration->parsemodels
           add Appkey
           ModelApp Bind
           set SubscriptionAddress
        since most of mesh configuration is wrapped from user, it takes about 20-30 seconds to setup the light.
    c> User will be notified on each provision complete, proxy connected/disconnected, configuration complete by a toast.
       once configuration complete toast is shown, user can start controling light.
 
6>  Click Settings icon on the home screen and select the option "Add BIG". This will provision BIG and add this device to the mesh network
    a> When prompted, user shall select the BD Address of BIG and add BIG's IP address
7>  "Inside home" usecase
    a> The MeshLighting app shall automatically assume that the user is "inside home". User can control brightness of individaul lights or an entire room.
    b> User can also add and remove lights/rooms inside home
8>  "Outside home" usecase
    a>  Once BIG is provisioned, ensure that the phone is also connected to the same WiFi AP as that of BIG
    b>  The app settings have an option to toggle between "Home" and "Away" modes.
    c>  When user selects "Away", the app will set the current transport to REST and disconnect an existing proxy connection. The light bulbs shall be managed via REST APIs.
    d>  User can control brightness of rooms/lights; however, user is not allowed to add/delete lights and room outside home


Build instructions
------------------
Android Application source and build procedure for the app shall be made available in a future release.

Limitations
-----------
The below limitations shall be fixed in the next version of the application
1>  Deletion of a created network is not supported
2>  Creation of groups within a room is not supported
3>  Multiple networks are not supported

Known Issue
-------------
Due to a known bug on android bt stack , the stack fails to establish connection and on Android logs connection error 133 occurs,
this happens due to enquiry db size issue.