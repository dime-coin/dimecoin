This utility is based on commands available in the .17 and .18 version of bitcoin-cli
This utility is intended to be run from the dimecoin folder containting your dimecoin-cli build and you should create a direcotory named data.
You'll want to edit the code to supply your destination wallet and/or passphrase for the consolidated transactions.
The utility will leave .json data in the data directory.  For good house keeping you may want to remove these files when finished with the utility.
Messing with your input transactions is risky and will cost you fees to transact.  Use this at your own risk!

Usage:
python3 iconsolidate.py