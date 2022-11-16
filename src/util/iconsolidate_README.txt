This utility is based on commands available in the .17 and .18 version of bitcoin-cli
This utility is intended to be run from the coin/src/util folder where your coin-cli is in coin/src.
Your coind daemon must be running and fully synced.
You should create a directory named data.
You'll want to edit the code to supply your destination wallet and/or passphrase for the consolidated transactions.
The utility will leave .json data in the data directory.  For good house keeping you may want to remove these files when finished with the utility.
Be absolutely certain you're using correct wallets.  If you send mainnet coin to a testnet wallet it will be lost forever!
Messing with your input transactions is risky and will cost you fees to transact.  Use this tool at your own risk!

Usage:
python3 iconsolidate.py
