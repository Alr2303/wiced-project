print("Start of script");

var ssid = "YOUR_AP_SSID";
var password = "YOUR_AP_PASSPHRASE";

function wifiConnectCallback()
{
    print("Connected to AP '"+ssid+"'");

    print("Getting details via 'wifi.getDetails()'");
    var details = this.getDetails();
    print("Returned value:");
    print("\tstatus:    "+details["status"]);
    print("\trssi:      "+details["rssi"]);
    print("\tssid:      "+details["ssid"]);
    print("\tpassword:  "+details["password"]);
    print("\tsavedSsid: "+details["savedSsid"]);

    print("Getting IP via 'wifi.getIP()'");
    var ip = this.getIP();
    print("Returned value:");
    print("\tip:      "+ip["ip"]);
    print("\tnetmask: "+ip["netmask"]);
    print("\tgw:      "+ip["gw"]);
    print("\tmac:     "+ip["mac"])

    print("Getting hostname via 'wifi.getHostname()'");
    var hostname = this.getHostname();
    print("Returned value:");
    print("\thostname: "+hostname);

    print("*** WIFI TEST PASSED ***");
};

function wifiScanCallback()
{
    print("Scan complete! Found "+arguments.length+" APs");
    for (var i = 0; i < arguments.length; i++)
    {
        print("["+i+"] "+arguments[i]["mac"]+" channel="+arguments[i]["channel"]+" authMode="+arguments[i]["authMode"]+" isHidden="+arguments[i]["isHidden"]+" SSID="+arguments[i]["ssid"]);
    }

    print("Connecting to AP via 'wifi.connect("+ssid+",{password:"+password+"},wifiConnectCallback)");
    this.connect(ssid,{password:password},wifiConnectCallback);
};

try {
    print("Creating new 'wifi' module variable wifi_module");
    var wifi_module = require("wifi");

    print("Getting details via 'wifi.getDetails()'");
    var details = wifi_module.getDetails();
    print("Returned value:");
    print("\tstatus:    "+details["status"]);
    print("\trssi:      "+details["rssi"]);
    print("\tssid:      "+details["ssid"]);
    print("\tpassword:  "+details["password"]);
    print("\tsavedSsid: "+details["savedSsid"]);

    print("Scanning via 'wifi.scan(wifiScanCallback)'");
    wifi_module.scan(wifiScanCallback);
} catch (e) {
    print( "Failed" );
}

print("End of script");
