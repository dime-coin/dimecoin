#########################################################################
##  DIMECOIN Master Node earnings estimator v0.0.1                     ##
##  Created by Dalamar 10/15/2022                                      ##
##  This utility is intended to provide an estimate of earnings.       ##
##  There are no guarentees of complete accuracy and this must not     ##
##  be considered financial advice.                                    ##
##  use this at your own risk!                                         ##
#########################################################################

'''
Formula is (n/t)*r*b*a
n = total number of masternodes an operator controls.
t = the total number of masternodes on the network
r = the current block reward
b = blocks in an average day
a = average masternode payment (%)
'''

import os
import json

class MNEstimator:
    def __init__(self):
        self.mnfile = "./data/mns.json"
        self.cli = "../dimecoin-cli"  # change this to ../dimecoin-cli -testnet if you're using testnet.
        self.testnet = False  # Set to True if using testnet

    def getmasternodes(self):
        if self.testnet == True:
            command = f"{self.cli} -testnet masternode list status \"ENABLED\""
        else:
            command = f"{self.cli} masternode list status \"ENABLED\""
        print(f"executing: {command}")
        mns = os.popen(command)
        masternode_list = mns.read()
        masternodes = json.loads(masternode_list) 
        # All Enabled master nodes are added to the masternode_list
        # Using a list comprehension we can search those results for a count of how many 'ENABLED' master nodes exist      
        matches = [match for match in masternodes.items() if "ENABLED" in match]
        #print(len(matches))
        mncount = len(matches)

        return mncount

    def getblocktemplate(self):
        if self.testnet == True:
            command = f"{self.cli} -testnet getblocktemplate"
        else:
            command = f"{self.cli} getblocktemplate"
        print(f"executing: {command}")
        blocktemplate = os.popen(command)
        blocktemplate_list = blocktemplate.read()
        btjson = json.loads(blocktemplate_list)
        # Extract the current block reward    
        blockreward = btjson["coinbasevalue"]
        blockreward = blockreward * .00001
        #print(str(blockreward))

        return blockreward

    def mncalc(self,n):
        t = self.getmasternodes()
        r = self.getblocktemplate()
        # Estimating 64 second block time would be about 1,440 blocks per 24hrs.
        b = input("Enter the average number of blocks in 24 hours (Leave blank for default 1440): ")
        if b == "":
            b = 1440
        a = input("Enter the average master node extraction percent ie .45 (Leave blank for default of 45%): ")
        if a == "":
            a = .45        

        reward_total = (int(n)/t)*r*int(b)*float(a)

        print(f"If you operate {n} masternode(s), there is a total of {t} enabled masternode(s), the current block reward is {r}, the average daily blocks is {b}, and the average masternode payment percent is {float(a) * 100}%")
        print(f"({n}/{t}) * {r} * {b} * {a}% = {reward_total}")
        reward_total = "{:,}".format(reward_total)
        print(f"The total ESTIMATED daily masternode reward is: {reward_total}")

    def startup(self):
        n = input("How many master nodes do you plan to operate? (Leave blank for default of 1): ")
        if n == "":
            n = 1
        self.mncalc(n)


if __name__ == '__main__':
    _dc = MNEstimator()
    welcome = f"""************************************************************************************************************
*   Welcome to the Master Node estimator tool.  This tool is intended to provide estimates only.           * 
*   This is not financial advice.                                                                          *
*   Use this utility at your own risk!                                                                     *
************************************************************************************************************"""
    print(welcome)
    _dc.startup()