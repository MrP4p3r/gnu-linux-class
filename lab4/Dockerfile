FROM ubuntu

COPY ./src/ /src/
COPY lab4.sh /

RUN mkdir /src/bin && chmod 755 /lab4.sh
RUN apt update && apt install make

WORKDIR /src

ENTRYPOINT /lab4.sh ; /bin/bash
