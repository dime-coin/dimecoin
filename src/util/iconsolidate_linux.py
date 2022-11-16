#########################################################################
##  DIMECOIN input consolidator v0.0.2                                 ##
##  Created by Dalamar 10/1/2022                                       ##
##  This utility is intended for consolidating input                   ##
##  transactions.                                                      ##
##  Calculations are delicate and can result in the loss of coin       ##
##  use this at your own risk!                                         ##
#########################################################################

import os, sys
import json
import time, datetime

class DimeConsolidator:
    def __init__(self):
        self.dest_wallet = ""    # Add your destination wallet here
        self.num_of_txns = 66  # don't change this number
        self.max_txns = 0
        self.fee = 0
        self.txn_id = ""
        self.hexoutput = ""
        self.passphrase = ""    # If you want you can hard code your passphrase.  Not recomended.  However it's useful for a large number of transactions.
        self.unspentfile = "./data/unspent.json"
        self.hexoutputfile = "./data/txnhexcode.json"
        self.walletinfofile = "./data/winfo.json"
        self.cli = "../dimecoin-cli"  # The coin cli executable goes here, change this to "../dimecoin-cli -testnet"  if using testnet.

    def log_output(self,fname, data):
            filepath = "./data/" + fname
            if os.path.isfile(filepath):
                pass
            try:
                file = open(filepath, 'w+')
                file.close()
                fd = os.open(filepath, os.O_RDWR)
                line = str.encode(data)
                numBytes = os.write(fd, line)
                print(f"Creating {filepath} bytes:{numBytes}")
                os.close(fd)
            except:
                print("Failed to write to ./data")

    def read_json(self,filename):
        try:
            with open(filename) as data_file:
                file_dict = json.load(data_file)
                return file_dict
        except Exception as e:
            print(f"There was a problem reading from: {filename}")

    def get_wallet_info(self):
        if self.testnet != False:
            command = f"{self.pathandCli} -testnet {self.datadir} getwalletinfo"
        else:
            command = f"{self.pathandCli} {self.datadir} getwalletinfo"
        print(f"executing: {command}")    
        winfo = os.popen(command)
        wallet_info = winfo.read()
        self.log_output("winfo.json", wallet_info)
        winfo_jdata = self.read_json(self.walletinfofile)
        wstatus = winfo_jdata["unlocked_until"]
        #print(wallet_info)
        if wstatus > 0:
            print("Wallet unlocked: TRUE")
            return True
        else:
            print("Wallet unlocked: FALSE")
            return False

    def consolidate_txns(self,jdata,num_of_txns):
        if len(jdata) > num_of_txns:
            txn_list_amnt = []
            txn_list_noamnt = []
            for i in range(num_of_txns):
                #print(jdata[i])
                txn_summary_amnt = {"txid":str(jdata[i]['txid']),"vout":int(jdata[i]['vout']),"scriptPubKey":str(jdata[i]['scriptPubKey']),"amount":float(jdata[i]['amount'])}
                txn_summary_noamnt = {"txid":str(jdata[i]['txid']),"vout":int(jdata[i]['vout']),"scriptPubKey":str(jdata[i]['scriptPubKey'])}
                txn_list_amnt.append(txn_summary_amnt)
                txn_list_noamnt.append(txn_summary_noamnt)
            print(json.dumps(txn_list_amnt, indent=4))
            return txn_list_amnt, txn_list_noamnt
        else:
            print(f"Less than {num_of_txns} unspent transactions available")

    def total_txn_amnts(self,txn_list_amnt, fee):
        sum = 0
        for i in range(len(txn_list_amnt)):
            #print(txn_list[i]["amount"])
            sum = sum + txn_list_amnt[i]["amount"]
        total = float(sum) - float(fee)
        print(f"total amount(s) {sum} minus {str(fee)} fee = {str(total)}")
        return total


    def create_txn(self,txn_list_noamnt, totalamnt):
        wallet_amnt = {self.dest_wallet:totalamnt}
        txn_list_noamnt = json.dumps(txn_list_noamnt)
        wallet_amnt = json.dumps(wallet_amnt)
        command = f"{self.cli} createrawtransaction '{txn_list_noamnt}' '{wallet_amnt}'"
        #print(command)
        createtxn = os.popen(command)
        txn_hex_code = createtxn.read()
        return txn_hex_code

    def unlock_wallet(self,passphrase, unlocktime):
        command = f"{self.cli} walletpassphrase {passphrase} {unlocktime}"
        unlock = os.popen(command)
        print(f"Unlocking wallet for {str(unlocktime)} seconds...")

    def sign_txn(self,txn_hex_code):
        command = f"{self.cli} signrawtransactionwithwallet {txn_hex_code}"
        #print(command)
        signtxn = os.popen(command)
        hexoutput =  signtxn.read()
        self.log_output("txnhexcode.json", hexoutput)
        hexoutput_jdata = self.read_json(self.hexoutputfile)
        return hexoutput_jdata

    def send_txn(self,signed_hex):
        command = f"{self.cli} sendrawtransaction {signed_hex}"
        #print(command)
        txn = os.popen(command)
        txn_id = txn.read()
        return txn_id

    def view_txn(self, txn_id):
        command = f"{self.cli} gettransaction {txn_id}"
        txn = os.popen(command)
        txn_output = txn.read()
        print(txn_output)

    def getbalance(self):
        command = f"{self.cli} getbalance"
        bal = os.popen(command)
        balance = bal.read()
        return balance

    def getunspent(self, maxinput):
        command = f"{self.cli} listunspent"
        unspent = os.popen(command)
        # query all unspent transactions and convert the stream to a list
        list_unspent = unspent.read()
        self.log_output("unspent.json", list_unspent)

        low_amount_list = []
        jdata = self.read_json(self.unspentfile)
        # parse the list of unspent transactions keeping only those under the requested amount
        for i in range(len(jdata)):
            if jdata[i]['amount'] <= int(maxinput):
                low_amount_list.append(jdata[i])
        if low_amount_list != "":
            lowamountjson = json.dumps(low_amount_list)
            self.log_output("unspent.json", lowamountjson)
            jdata = self.read_json(self.unspentfile)
            if len(jdata) <= 0:
                print(f"Failed to find any inputs with an amount below: {str(maxinput)}")
                sys.exit(0)
        else:
            print(f"Failed to find any inputs with an amount below: {str(maxinput)}")
            sys.exit(0)
        return jdata


    def startup(self):
        unlocktime = 60
        balance = self.getbalance()
        print(f"Current wallet balance is: {balance}")
        maxinput = input("What is the max input amount you'd like selected to consolidate (i.e. 10000): ")
        jdata = self.getunspent(maxinput)
        self.max_txns = int(len(jdata))
        print(f"You have {len(jdata) - 1} unspent transactions.")
        if (len(jdata) - 1) == 0:
            sys.exit(0)
        # TODO: rewrite this to scale based on how many transactions exist or by how many the user specifies
        if self.max_txns >= 1320:
            loop = input("Do you want to loop 20 times? (1320 inputs) y/n:")
            loopqty = 20
        elif self.max_txns >= 660:
            loop = input("Do you want to loop 10 times? (660 inputs) y/n: ")
            loopqty = 10
        elif self.max_txns >= 330:
            loop = input("Do you want to loop 5 times? (330 inputs) y/n: ")
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
                self.unlock_wallet(self.passphrase,unlocktime)
                wstatus = True
            for x in range(loopqty):
                print(f"Starting loop number {x+1} of {str(loopqty)}")
                self.num_of_txns = 60
                self.main(jdata, wstatus, maxinput)
                self.txn_id = self.send_txn(str(self.hexoutput['hex']))
                if self.txn_id != "":
                    print(f"********** SUCCESS! **********\nTransaction id: {self.txn_id}")
                time.sleep(5)
            else:
                print("Finsihed!")
        else:
            self.num_of_txns = input(f"How many transactions would you like to combine? (max {self.max_txns - 1}): ")
            if self.num_of_txns == "" or int(self.num_of_txns) <= 0:
                print("Invalid number of transactions provided!")
                sys.exit(0)
            if self.passphrase == "":
                wstatus = self.get_wallet_info()
            else:
                self.unlock_wallet(self.passphrase, unlocktime)
                wstatus = True
            self.main(jdata,wstatus, maxinput)

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
        if self.passphrase == "" or wstatus == False:
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
************************************************************************************************************"""
    print(welcome)
    _dc.startup()
