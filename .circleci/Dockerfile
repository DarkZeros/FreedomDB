FROM cimg/base:stable

RUN sudo apt-get update && sudo apt-get install -y gcc g++ build-essential cmake telnet
RUN sudo apt-get update && sudo apt-get install -y python3 python3-pip python3-distutils
RUN sudo apt-get update && sudo apt-get install -y libpython3-dev libboost-dev libpq-dev libpqxx-dev ninja-build

RUN pip3 install pytest
