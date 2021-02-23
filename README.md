# Freedom DB

Blockchain powered distributed PostgreSQL database

# What is this?

It is a distributed SQL database you can interact with SQL commands.
Multiple clients over P2P share the SQL transactions executed to the DB.
Consensus on the order of how to apply the SQl transactions is achieved with a blockchain.
Usage of the DB (space used by users) as well as transactions amount (cost of transactions) is paid with an internal crypto coin stored in the (DB).
Security in the DB state is done with Blockchain, security in permissions of each user is based on PostgreSQL permissions.

# Wait another cryptocurrency?

NO.
This is a pain and simple a SQL database, just distributed, and secure.

The Blockchain is for consensus only.
The currency is to avoid missuses of the DB.

# Why do I need this?

All cryptocurrencies have to store the ledger. Thats a big amount of data (many Terabytes)
They can't delete any bit of it, since it is needed to keep track of coins.
They have to keep track of all transactions done in the past.
For this reason, they just keep a few bytes per transaction, storing data costs a lot, since it stays forever.

In an SQL DB transactions are forgotten after executed, only the DB state matters. It is maintainable in the future.
This by itself enables the SQL database to hold much less space than Bitcoin/Eth in the long term.
Encorages users to delete unneeded data, and to perform little transactions, but if they want to do many, they can.


# What technologies it has?

- PostgreSQL(libpbxx): Main Database, contains all the data and executes the transactions. Also ensures permissions.
- MsgPack: For the P2P exchange of SQL Transactions performed by clients
- Cryptopp: Glue that ensures crypto security in transactions, blockchain, and DB hashing
- Pybind11: Provides C++/Python interfaces

# How to build and develop ?

* sudo aptitude install libpython-dev libboost-dev libpq-dev libpqxx-dev ninja-build
* Use cmake manually or setup.py