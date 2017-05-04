FROM ubuntu

COPY ./src/ /src/
COPY lab4.sh /

RUN mkdir /src/bin && chmod 755 /lab4.sh
RUN apt update && apt install -y make gcc

WORKDIR /src

ENTRYPOINT /lab4.sh ; /bin/bash
