FROM cimg/base:stable

# Requirements for building
RUN sudo apt-get update && sudo apt-get install -y gcc g++ build-essential cmake ninja-build

# Requirements for building python
RUN sudo apt-get update && sudo apt-get install -y python3 python3-pip python3-distutils

# Requirement for linking
RUN sudo apt-get update && sudo apt-get install -y libpython3-dev libboost-dev libpq-dev libpqxx-dev

# Test tools for the unit tests
RUN sudo apt-get update && sudo apt-get install -y telnet
RUN pip3 install pytest

# Install PostgreSQL and create a freedomdb user
RUN sudo apt-get update && sudo apt-get install -y postgresql postgresql-contrib
RUN sudo su -c "service postgresql start && sudo -u postgres psql -c \"CREATE USER freedomdb WITH SUPERUSER CREATEDB CREATEROLE PASSWORD 'freedomdb';\""

ENTRYPOINT sudo service postgresql start && /bin/bash
