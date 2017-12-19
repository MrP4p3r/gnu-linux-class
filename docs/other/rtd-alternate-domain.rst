.. _rtd-alternate-domain:

======================================
RTD документация на своем домене с SSL
======================================

О чем речь?
===========

В этом гайде описан пример действий, позволяющий привязать свой домен
к документации, размещенной на Read the Docs. Дисклеймер: наверняка, это
не самый красивый и простой способ, но оно работает.

Что используется?
=================

1. `Acme.sh <https://github.com/Neilpang/acme.sh>`_
2. nginx
3. bind9

Исходные данные
===============

1. Есть проект на GitHub, привязанный к проекту на Read the Docs
2. Есть виртуальный сервер (VPS) с Ubuntu 16.04
3. Есть свой (бесплатный) домен
4. На VPS установлен bind9 и nginx

Что же делать?
==============

Предполагается, что читатель знаком с основами конфигурирования nginx, bind9, и
что на сервере уже прописана информация о доменной зоне. А еще все
команды, описанные тут, выполняются из-под root.

Разберем ситуацию на примере текущего домена ``hlebushe.gq``.
В nginx *уже* есть пара виртуальных хостов: hlebushe.gq, files.hlebushe.gq.
Документацию планируется добавить на виртуальный хост gnu-linux.hlebushe.gq.

В первую очередь нужно прописать в файл с информацией о доменной зоне
(например ``/etc/bind/db.hlebushe.gq``) необходимо прописать поддомен
и IP-адрес сервера, на котором поднят nginx::

    gnu-linux    A    62.109.3.197

После этого нужно создать конфиг виртуального хоста. На сервере с указанным
в зоне адресом выполняем::

    cd /etc/nginx/sites-enabled
    touch /etc/nginx/sites-available/gnu-linux.hlebushe.gq
    ln -s /etc/nginx/sites-available/gnu-linux.hlebushe.gq ./
    editor ./gnu-linux.hlebushe.gq

В редакторе прописываем::

    server {
        listen 80;
        server_name gnu-linux.hlebushe.gq;

        location / {
            return 301 https://$host$request_uri;
        }

        location /.well-known/acme-challenge {
            alias /var/www/gnu-linux.hlebushe.gq/.well-known/acme-challenge;
        }
    }

Перезагружаем bind9 и nginx::

        systemctl restart bind9
        systemctl restart nginx

Тут может возникнуть сложность, если прописан catchall виртуальный хост,
который перенаправляет абсолютно все подключения по протоколу http на https.
Проблема гуглится или решается интуитивно. Кроме этого, при определенном
варианте конфигурации виртуальных хостов при подключении к одному будет
отображаться другой. Такого можно наверняка избежать, если во всех виртуальных
хостах в параметре ``listen`` прописывать порт, а в ``server_name`` имя
виртуального хоста и ничего лишнего.

Сохраняем конфиг. Теперь необходимо создать директорию из последнего блока.
Она может быть любой, лишь бы nginx имел права на чтение из нее. Веб-сервер
у меня запущен под пользователем ``nginxuser``. Делаем следующее::

    mkdir -p /var/www/gnu-linux.hlebushe.gq/.well-known/acme-challenge
    chown nginxuser -R /var/www/gnu-linux.hlebushe.gq

Эта директория нужна для acme.sh. Если не установлен acme.sh::

    wget -O -  https://get.acme.sh | sh

Необходимо залогиниться/перелогиться, чтобы появился alias к acme.sh.
Например, можно так::

    bash -l


Теперь организуем себе SSL сертификат на поддомен::

    # Директория для сертификатов
    mkdir -p /etc/sslcerts/gnu-linux.hlebushe.gq

    # Запрашиваем сертификат
    acme.sh --issue -d gnu-linux.hlebushe.gq -w /var/www/gnu-linux.hlebushe.gq

    # Помещаем его в нашу директорию
    acme.sh --install-cert \
        -d gnu-linux.hlebushe.gq \
        --cert-file /etc/sslcerts/gnu-linux.hlebushe.gq/cert.pem \
        --key-file /etc/sslcerts/gnu-linux.hlebushe.gq/key.pem \
        --reloadCmd 'systemctl restart nginx'

Очевидно, параметр ``--reloadCmd`` должен соответствовать используемой ОС.
В моем случае acme.sh запускается root пользователем (и cron задача, которую создает acme.sh для обновления сертификатов, тоже выполняется root пользователем), то и владельцем файлов сертификата является root. Серверу nginx необходимо иметь право хотя бы на чтение файлов сертификата. Поэтому я сделал что-то вроде::

    groupadd web
    usermod -aG web nginxuser

    CERTDIR=/etc/sslcerts/gnu-linux.hlebushe.gq
    chown -R :web $CERTDIR
    chmod 640 -R $CERTDIR; chmod ug+X -R $CERTDIR

Теперь нужно дополнить конфиг виртуального хоста следующим::

    server {
        listen 443 ssl;
        server_name gnu-linux.hlebushe.gq;

        ssl_certificate /etc/sslcerts/gnu-linux.hlebushe.gq/cert.pem;
        ssl_certificate_key /etc/sslcerts/gnu-linux.hlebushe.gq/key.pem;

        location / {
            proxy_pass https://mrp4p3r-gnu-linux-class-spring-2017.readthedocs.io:443;
            proxy_set_header Host $http_host;
            proxy_set_header X-Forwarded-Proto https;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Scheme $scheme;
            proxy_set_header X-RTD-SLUG mrp4p3r-gnu-linux-class-spring-2017;
            proxy_connect_timeout 10s;
            proxy_read_timeout 20s;
        }
    }

Внимание: предварительно нужно добавить свой домен в настройках (Admin > Domains)
своего проекта на readthedocs.io.
Параметр proxy_pass содержит ссылку на readthedocs.io, по которой раздается документация.
Параметр proxy_set_header X-RTD-SLUG должен содержать т. н. slug проекта.
Он соответствует имени проекта, если последний не был переименован.

Выполняем::

    systemctl restart nginx

После этого все должно заработать, если нет ошибок с правами и конфигами.
