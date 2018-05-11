/*
 * Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of Cypress Semiconductor
 *  Corporation. All rights reserved. This software, including source code, documentation and  related
 * materials ("Software"), is owned by Cypress Semiconductor  Corporation or one of its
 *  subsidiaries ("Cypress") and is protected by and subject to worldwide patent protection
 * (United States and foreign), United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software ("EULA"). If no EULA applies, Cypress
 * hereby grants you a personal, nonexclusive, non-transferable license to  copy, modify, and
 * compile the Software source code solely for use in connection with Cypress's  integrated circuit
 * products. Any reproduction, modification, translation, compilation,  or representation of this
 * Software except as specified above is prohibited without the express written permission of
 * Cypress. Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO  WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING,  BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to
 * the Software without notice. Cypress does not assume any liability arising out of the application
 * or use of the Software or any product or circuit  described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or failure of the
 * Cypress product may reasonably be expected to result  in significant property damage, injury
 * or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the
 *  manufacturer of such system or application assumes  all risk of such use and in doing so agrees
 * to indemnify Cypress against all liability.
 */

import UIKit
import CoreBluetooth

protocol ScanViewControllerDelegate {
    func scanningComplete(_ data: Bool)
}

class ScanningConfigureViewController: UIViewController, ConnectPeripheralProtocol, ReadPeripheralProtocol {
    
    var serviceUUIDString:String        = "1B7E8251-2877-41C3-B46E-CF057C562023"
    var peripheralUUIDString:String     = "7EC612F7-F388-0284-624C-BE76C314C6CD"
    var characteristicUUIDString:String = "B6251F0B-3869-4C0D-ACAB-D93F45187E6F"

    var peripheralInProximity : Bool = false
    var timer = Timer()
    var writeCount = 0
    var firstPeripheralCount = 0;
    var data: Bool = false;


    @IBOutlet weak var spinner: UIActivityIndicatorView!
    weak var currentViewController: UIViewController?
    var delegate: ScanViewControllerDelegate?

    override func viewDidLoad() {
        super.viewDidLoad()

        spinner.startAnimating()

        initBluetooth()

        checkforWifi()
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    func checkforWifi() {

        let ssidName = SSIDNetwork()

        if let temp = ssidName.getSSID() {
            print("connected to network \(temp)")
            startScanning()

        } else {

            //let alertView = UIAlertController(title: "SSID Check", message: "Not connected to any network. Kindly connect and then Try!", preferredStyle: .Alert)
            //alertView.addAction(UIAlertAction(title: "Ok", style: .Default, handler: nil))
            print("NOT CONNECTED TO NETWORK.KINDLY CONNECT!")

        }
    }

    func isPeripheralinProximity(_ peripheral : Peripheral, RSSI:NSNumber ) -> Bool {
        
        self.peripheralInProximity = false
        
        //Reject any where the value is above the reasonable range
        if RSSI.intValue > -15 {
            self.peripheralInProximity = false
            return self.peripheralInProximity
        
        }
        
        // Reject if the signal strength is too low to be close enough
        
        if RSSI.intValue < -40 {
            self.peripheralInProximity = false
            return self.peripheralInProximity
        }
        
        self.peripheralInProximity = true
        
        return self.peripheralInProximity
    }
    
    func initBluetooth() {
        
        CentralManager.sharedInstance().connectPeripheralDelegate = self
    }
    
    // MARK: ConnectPeripheralProtocol
    func didConnectPeripheral(_ cbPeripheral: CBPeripheral!) {
        
    }
    
    func didDisconnectPeripheral(_ cbPeripheral: CBPeripheral!, error: NSError!, userClickedCancel: Bool) {
        Logger.debug("AppDelegate#didDisconnectPeripheral \(cbPeripheral.name)" as AnyObject)
    }
    
    func didRestorePeripheral(_ peripheral: Peripheral) {
        Logger.debug("AppDelegate#didRestorePeripheral \(peripheral.name)" as AnyObject)
    }
    
    func bluetoothBecomeAvailable() {
        print("calling start scan")
        self.startScanning()
    }
    
    func bluetoothBecomeUnavailable() {
        print("calling stop scan")
        self.stopScanning()
    }
    
    // MARK: Public functions

    func startScanning() {
        print("startScanning")

        for peripheral:Peripheral in DataManager.sharedInstance().discoveredPeripherals.values {
            peripheral.isNearby = false
        }
        CentralManager.sharedInstance().startScanning(afterPeripheralDiscovered, allowDuplicatesKey: true)
    }
    
    func stopScanning() {
         print("stopScanning")
        CentralManager.sharedInstance().stopScanning()
    }
    
    func afterPeripheralDiscovered(_ cbPeripheral:CBPeripheral, advertisementData:NSDictionary, RSSI:NSNumber) {
        
        Logger.debug("AppDelegate#afterPeripheralDiscovered: \(cbPeripheral)" as AnyObject)
        
        var peripheral : Peripheral
        
        if let p = DataManager.sharedInstance().discoveredPeripherals[cbPeripheral] {
            
            peripheral = p
            
            DispatchQueue.main.async(execute: {

                if self.isPeripheralinProximity(peripheral, RSSI: NSNumber(value:RSSI.intValue)) == true {
                    print("inside \(RSSI)")

                    
                    if CentralManager.sharedInstance().isScanning == true {

                    self.spinner.stopAnimating()
                    self.stopScanning()
                    self.notifyCallingViewController()

                    }
                } else {
                    print("device is not in proximity \(RSSI)")
                }
            })
            
        }
        else {

            peripheral = Peripheral(cbPeripheral:cbPeripheral, advertisements:advertisementData as Dictionary<NSObject, AnyObject>, rssi:RSSI.intValue)
            
            DataManager.sharedInstance().discoveredPeripherals[peripheral.cbPeripheral] = peripheral
            
        }
        
        peripheral.isNearby = true
        
        NotificationCenter.default.post(name: Notification.Name(rawValue: "afterPeripheralDiscovered"), object: nil)
    }
    
    // MARK: ReadPeripheralProtocol
    
    func didDiscoverCharacteristicsofPeripheral(_ cbservice : CBService!) {
        
    }
    
    func didWriteValueForCharacteristic(_ cbPeripheral: CBPeripheral!, characteristic: CBCharacteristic!, error: NSError!) {
        
    }
    
    func didUpdateValueForCharacteristic(_ cbPeripheral: CBPeripheral!, characteristic: CBCharacteristic!, error: NSError!) {
        
    }

    func notifyCallingViewController() {
        data = true
        print("notifyCallingViewController scanComplete")
        self.delegate?.scanningComplete(data)
    }
}
