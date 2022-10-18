#########################################################################
##  DIMECOIN input consolidator for Windows v0.0.1                     ##
##  Created by Dalamar 9/3/2022                                        ##
##  This utility is intended for consolidating input                   ##
##  transactions.                                                      ##
##  Calculations are delicate and can result in the loss of coin       ##
##  use this at your own risk!                                         ##
#########################################################################

import os, sys
import json
import time

class DimeConsolidator:
    def __init__(self):
        self.dest_wallet = ""    # Add your destination wallet here
        self.num_of_txns = 27    # don't change this number, windows command prompt has a character limitation. 
        self.max_txns = 0
        self.fee = 0
        self.txn_id = ""
        self.hexoutput = ""
        self.passphrase = ""   # If you want you can hard code your passphrase.  Not recomended.  However it's useful for a large number of transactions.
        self.defaultCliPath = "C:\Program Files\Dimecoin\daemon"  # Path where your cli.exe is located i.e C:\Program Files\Dimecoin\daemon
        self.datadir = ""   # Leave this blank if you're not using the -datadir argument. i.e -datadir=C:\Program Files\Dimecoin\Blockchain
        self.defaultCliExe = "dimecoin-cli.exe"         # the cli.exe file
        self.pathandCli = self.defaultCliPath + "\\" + self.defaultCliExe
        self.testnet = True
        self.unspentfile = self.defaultCliPath + "\\data\\unspent.json"
        self.hexoutputfile = self.defaultCliPath + "\\data\\txnhexcode.json"
        self.walletinfofile = self.defaultCliPath + "\\data\\winfo.json"
        self.txnprepfile = self.defaultCliPath + "\\data\\txnprep.json"
        self.testfile = self.defaultCliPath + "\\data\\test.json"
        self.unencrypted = False
        self.wstatus = False

    def log_output(self,fname, data, method):
        #Store API results in a file
        filepath = fname
        if os.path.isfile(filepath):
            pass
        try:
            file = open(filepath, method)
            file.close()
            fd = os.open(filepath, os.O_RDWR)
            line = str.encode(data)
            numBytes = os.write(fd, line)
            print(f"Creating {filepath} bytes:{numBytes}")
            os.close(fd)
        except:
            print("Failed to write to ./data")

    def read_json(self,filename):
        # Retrieve json data from a file
        try:
            with open(filename) as data_file:
                file_dict = json.load(data_file)
                return file_dict
        except Exception as e:
            print(f"There was a problem reading from: {filename}")

    def get_wallet_info(self):
        # Get wallet info to determine if it's currently locked.
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} getwalletinfo"
        else:
            command = f"{self.pathandCli} {self.datadir} getwalletinfo"
        print(f"executing: {command}")
        winfo = os.popen(command)
        wallet_info = winfo.read()
        self.log_output(self.walletinfofile, wallet_info, 'w+')
        winfo_jdata = self.read_json(self.walletinfofile)

        try:
             self.wstatus = winfo_jdata["unlocked_until"]
        except KeyError: 
             print("Error unlocking wallet.  Is your wallet encrypted?")
             print("Proceeding with unencrypted wallet...")
             self.unencrypted = True
             self.wstatus = 1

        #print(wallet_info)
        if self.wstatus > 0:
            print("Wallet unlocked: TRUE")
            return True
        else:
            print("Wallet unlocked: FALSE")
            return False

    def consolidate_txns(self,jdata,num_of_txns):
        # Extract and format all inputs targeted for consolidation
        if len(jdata) > num_of_txns:
            txn_list_amnt = []
            txn_list_noamnt = []
            txn_summary_string = ""
            for i in range(num_of_txns):
                txn_summary_amnt = {"txid":str(jdata[i]['txid']),"vout":int(jdata[i]['vout']),"scriptPubKey":str(jdata[i]['scriptPubKey']),"amount":float(jdata[i]['amount'])}
 
                # For some reason escape chracters don't come through correctly with the cli when using a standard dict in windows.
                # Due to this complexity, we're manually concatenating strings to preserve escape characters.
                txn_summary_noamnt = r"{" + "\\" + '"txid\\"' + ":\\" + '"' + str(jdata[i]['txid']) + '\\"' + ',\\' + '"vout\\"' + ":" + str(int(jdata[i]['vout'])) + ',\\' + '"scriptPubKey\\"' + ":\\" + '"' + str(jdata[i]['scriptPubKey']) + '\\"' + r"}"

                # Append each input
                txn_summary_string = txn_summary_string + txn_summary_noamnt
            
                txn_list_amnt.append(txn_summary_amnt)

            # Make sure each input is seprated with a comma.
            txn_summary_string = txn_summary_string.replace('}{','},{')
            #self.temp = txn_summary_string
            txn_list_noamnt = txn_summary_string

            return txn_list_amnt, txn_list_noamnt
        else:
            print(f"Less than {num_of_txns} unspent transactions available")

    def total_txn_amnts(self,txn_list_amnt, fee):
        # Identify the total coin quantity for all targeted inputs.
        sum = 0
        #print(json.dumps(txn_list_amnt))
        for i in range(len(txn_list_amnt)):
            sum = sum + txn_list_amnt[i]["amount"]
        total = float(sum) - float(fee)
        print(f"total amount(s) {sum} minus {str(fee)} fee = {str(total)}")
        return total

    def create_txn(self,txn_list_noamnt, totalamnt):
        # Create the raw transaction for all targeted inputs and coin amounts.
        #For some reason escape chracters don't come through correctly with the cli when using a standard dict in windows.
        #Due to this complexity, we're manually concatenating strings to preserve escape characters.
        wallet_amnt = r"{" + "\\" + '"' + self.dest_wallet + "\\" +  '"' + ":" + str(totalamnt) + "}"
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} createrawtransaction \"[{txn_list_noamnt}]\" \"{wallet_amnt}\""
        else:
            command = f"{self.pathandCli} {self.datadir} createrawtransaction \"[{txn_list_noamnt}]\" \"{wallet_amnt}\""
        print(command)
        createtxn = os.popen(command)
        txn_hex_code = createtxn.read()
        return txn_hex_code

    def unlock_wallet(self,passphrase, unlocktime):
        # Unlock the wallet so transactions can be sent
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} walletpassphrase {passphrase} {unlocktime}"
        else:
            command = f"{self.pathandCli} {self.datadir} walletpassphrase {passphrase} {unlocktime}"
        unlock = os.popen(command)
        print(f"Unlocking wallet for {str(unlocktime)} seconds...")

    def sign_txn(self,txn_hex_code):
        # Sign the transaction
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} signrawtransactionwithwallet {txn_hex_code}"
        else:
            command = f"{self.pathandCli} {self.datadir} signrawtransactionwithwallet {txn_hex_code}"
        signtxn = os.popen(command)
        print("Signing transaction...")
        hexoutput =  signtxn.read()
        self.log_output(self.hexoutputfile, hexoutput, 'w+')
        hexoutput_jdata = self.read_json(self.hexoutputfile)
        return hexoutput_jdata

    def send_txn(self,signed_hex):
        # Send the signed transaction
        if self.testnet != "":
            command = f"{self.pathandCli} -testnet {self.datadir} sendrawtransaction {signed_hex}"
        else:
            command = f"{self.pathandCli} {self.datadir} sendrawtransaction {signed_hex}"
        print("Sending transaction...")
        txn = os.popen(command)
        txn_id = txn.read()
        return txn_id

    def view_txn(self, txn_id):
        # Optional feature to view the transaction after it's been sent.
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} gettransaction {txn_id}"
        else:
            command = f"{self.pathandCli} {self.datadir} gettransaction {txn_id}"
        print(f"executing: {command}")
        txn = os.popen(command)
        txn_output = txn.read()
        print(txn_output)

    def getbalance(self):
        # Query the wallet balance
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} getbalance"
        else:
            command = f"{self.pathandCli} {self.datadir} getbalance"
        print(f"executing: {command}")
        bal = os.popen(command)
        balance = bal.read()
        return balance

    def getunspent(self, maxinput):
        # Locate all unspent and unlocked transactions for consolidation
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} listunspent"
        else:
            command = f"{self.pathandCli} {self.datadir} listunspent"
        print(f"executing: {command}")
        unspent = os.popen(command)
        # convert the stream to a list
        list_unspent = unspent.read()
        self.log_output(self.unspentfile, list_unspent, 'w+')

        low_amount_list = []
        jdata = self.read_json(self.unspentfile)
        # parse the list of unspent transactions keeping only those under the requested amount
        for i in range(len(jdata)):
            if jdata[i]['amount'] <= int(maxinput):
                low_amount_list.append(jdata[i])
        if low_amount_list != "":
            lowamountjson = json.dumps(low_amount_list)
            self.log_output(self.unspentfile, lowamountjson, 'w+')
            jdata = self.read_json(self.unspentfile)
            if len(jdata) <= 0:
                print(f"Failed to find any inputs with an amount below: {str(maxinput)}")
                sys.exit(0)
        else:
            print(f"Failed to find any inputs with an amount below: {str(maxinput)}")
            sys.exit(0)
        return jdata

    def confirmation(self, info):
        confirm = input(f"Are you absolutely sure this {info} is correct? (Y/N) :")
        if confirm.upper() == "Y":
            return True
        else:
            return False

    def startup(self):
        unlocktime = 60
        cliPath = input("\nEnter the full path to your coin cli. i.e. C:\Program Files\Dimecoin\daemon (leave blank for default install path): ")
        if cliPath == "":
            cliPath = self.defaultCliPath
            print(f"using: {self.defaultCliPath}")
        else:
            self.defaultCliPath = cliPath
            print(f"using: {self.defaultCliPath}")

        cliExe = input("Enter your cli .exe file name. i.e. dimecoin-cli.exe (leave blank for default exe file): ")
        if cliExe == "":
            cliExe = self.defaultCliExe
            print(f"using: {self.defaultCliExe}")
        else:
            self.defaultCliExe = cliExe
            print(f"using: {self.defaultCliExe}")

        testnetEnabled = input("Will you be using testnet? (leave blank for Y): ")
        if (testnetEnabled.upper() == "" or testnetEnabled.upper() == "Y"):
            self.testnet = True
            print(f"using: -testnet")
        else:
            self.testnet = False
            print("*** WARNING *** USING PRODUCTION!!!")

        rcv_wallet = input(f"Enter the destination wallet address.  When inputs are consolidated they'll need to be sent to yourself to a valid receiving address. Pres enter to use {self.dest_wallet}: ")
        if rcv_wallet == "":
            rcv_wallet = self.dest_wallet
            if self.dest_wallet == "":
                print("Error! No wallet provided.  Closing app!")
                sys.exit(0)
        else:
            conf = self.confirmation(self.dest_wallet)
            if conf == True:
                print(f"using destination wallet: {self.dest_wallet}")
                pass
            else:
                self.dest_wallet = input("Enter the destination wallet address again. :")
                conf = self.confirmation(self.dest_wallet)
                if conf == True:
                    pass
                else:
                    print("Error! Bad or no wallet provided.  Closing app!")
                    sys.exit(0)
        
        dataFolder = self.defaultCliPath + "\\data"
        os.popen(f"mkdir {dataFolder}")
        print(f"using: {dataFolder}")

        try:
            balance = self.getbalance()
            print(f"Current wallet balance is: {balance}")
        except Exception as e:
            print("*** ERROR ***")
            print("Is your daemon running?")
            print(f"{e}")
            sys.exit(0)

        maxinput = input("What is the max input amount you'd like selected to consolidate (i.e. 10000): ")
        if maxinput == "":
            print("No input size prefrence provided, defaulting to include inputs up to 1 billion in size")
            maxinput = 999999999
        jdata = self.getunspent(maxinput)
        self.max_txns = int(len(jdata))
        print(f"You have {len(jdata) - 1} unspent transactions.")
        if (len(jdata) - 1) == 0:
            sys.exit(0)    
        # TODO: rewrite this to scale based on how many transactions exist or by how many the user specifies
        if self.max_txns >= 540:
            loop = input("Do you want to loop 20 times? (540 inputs) y/n:")
            loopqty = 20
        elif self.max_txns >= 270:
            loop = input("Do you want to loop 10 times? (270 inputs) y/n: ")
            loopqty = 10
        elif self.max_txns >= 135:
            loop = input("Do you want to loop 5 times? (135 inputs) y/n: ")
            loopqty = 5
        else:
            loop = "n"
            loopqty = 1
            unlocktime = 60
        if loop.lower() == "y":
            if self.passphrase == "":
                wstatus = self.get_wallet_info()
            else:
                # Using unlocktime to scale how long to unlock the wallet based on number of inputs being consolidated.
                # The wallet needs to remain unlocked for the duration of the consolidation cycle.
                unlocktime = loopqty * 60
                self.unlock_wallet(self.passphrase, unlocktime)
                wstatus = True
            for x in range(loopqty):
                print(f"Starting loop number {x+1} of {str(loopqty)}")
                self.num_of_txns = 27
                self.main(jdata, wstatus, maxinput)
                self.txn_id = self.send_txn(str(self.hexoutput['hex']))
                if self.txn_id != "":
                    print(f"********** SUCCESS! **********\nTransaction id: {self.txn_id}")
                time.sleep(5)
            else:
                print("Finsihed!")
        else:
            # Windows command line has a limitation on the number of characters so we cap inputs at 27.
            self.num_of_txns = input(f"How many transactions would you like to combine? (max 27): ")
            if self.num_of_txns == "" or int(self.num_of_txns) <= 0 or int(self.num_of_txns) > 27:
                print("Invalid number of transactions provided!")
                sys.exit(0)
            if self.unencrypted == True:
                self.wstatus = True
            elif self.passphrase == "" and self.wstatus == False:
                self.wstatus = self.get_wallet_info()
            else:
                self.unlock_wallet(self.passphrase, unlocktime)
                self.wstatus = True

            self.main(jdata, self.wstatus, maxinput)

            go = input("Make sure everything looks correct.\nProceed? y/n: ")
            if go.lower() == "y":
                self.txn_id = self.send_txn(str(self.hexoutput['hex']))
            if self.txn_id != "":
                print(f"********** SUCCESS! **********\nTransaction id: {self.txn_id}")
                view = input("View this transactions y/n: ")
                if view.lower() == "y":
                    self.view_txn(self.txn_id)
                else:
                    sys.exit(0)
            else:
                print("Error something is wrong with this transaction.  Fee too small? Wallet unlock time expired?  Please try again.")
                sys.exit(0)

    def main(self,jdata, wstatus, maxinput):
        unlocktime = 60
        jdata = self.getunspent(maxinput)
        print(f"You have {len(jdata) - 1} unspent transactions.")
        if int(self.num_of_txns) < 10:
            self.fee = 0.01
        else:
            self.fee = int(self.num_of_txns) * 0.0015

        if int(self.num_of_txns) > 0 and int(self.num_of_txns) < self.max_txns:
            txn_list_amnt, txn_list_noamnt = self.consolidate_txns(jdata,int(self.num_of_txns))

        else:
            print("Invalid number of transactions!")
            sys.exit(0)

        totalamnt = self.total_txn_amnts(txn_list_amnt, float(self.fee))
        txn_hex_code = self.create_txn(txn_list_noamnt, totalamnt)
        print(f"Generating Transaction Hex Code...")

        # Need to unlock wallet before txn can be signed
        if self.unencrypted == True:
            print("Wallet unencrypted, no passphrase needed...")
        else:
            if self.passphrase == "" or wstatus == False: #or self.unencrypted == False:
                print("We must unlock your wallet to sign the transaction.")
                self.passphrase = input("Enter your wallet passphrase:")
                self.unlock_wallet(self.passphrase, unlocktime)

        print("Building transaction...")
        self.hexoutput = self.sign_txn(txn_hex_code)
        #print(self.hexoutput)
        print("Generating your Signed Hex Output...")

if __name__ == '__main__':
    _dc = DimeConsolidator()
    welcome = f"""************************************************************************************************************
*   Welcome to the cli input consolidation tool.  This tool can combine inputs into single transactions.   * 
*   Changing coin inputs can be delicate and if not done correctly can result in coin loss.                *
*   Use this utility at your own risk!                                                                     *
*   We'll start with a series of questions to begin...                                                     *
************************************************************************************************************"""
    print(welcome)
    _dc.startup()

