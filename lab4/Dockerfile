FROM ubuntu

COPY ./src /src
COPY lab4.sh /

RUN apt update && apt install make && chmod 755 /lab4.sh

WORKDIR /src

ENTRYPOINT /lab4.sh ; /bin/bash
