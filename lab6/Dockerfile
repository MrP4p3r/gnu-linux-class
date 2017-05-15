FROM ubuntu

RUN apt update && apt install -y make gcc

COPY ./src/ /src/
COPY ./lab6.sh /lab6.sh

RUN cd /src && \
    make clean && \
    make install && \
    chmod +x /lab6.sh

WORKDIR /src

CMD /lab6.sh
