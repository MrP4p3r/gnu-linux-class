.. _k-gnu-linux:

=======================================
Курсовая работа по дисциплине GNU/Linux
=======================================

Цели
====

В работе рассмотрены::

    1. Настройка службы DNS на сервере под управлением ОС GNU/Linux.
    2. Настройка веб-сервера.
    2. Описание службы запуска и остановки работы веб-приложения.
    3. Настроить доступ к веб-приложению и документации через соответствующие доменные имена.
    4. Получить SSL сертификаты для доступа к веб-приложению и документации через протокол HTTPS.

Используемые технологии
=======================

Имеется в наличии VPS сервер работающий на базе OpenVZ с ядром Linux 2.6.32.
Сильно ресурсов на сервере не требуется, поэтому был арендован самый дешевый
доступный вариант. На сервере установлен дистрибутив Linux 16.04.

Выполнять роль DNS службы будет Bind9. В качестве веб-сервера был выбрал
Nginx из-за его простой настройки.

Имеющееся реализованное на Python WSGI приложение, будем запускать с помощью
`Gunicorn <http://gunicorn.org/>`_. Gunicorn 'Green Unicorn' - легковесный
HTTP сервер WSGI приложений для Linux. Gunicorn имеет встроенный балансировщик
по воркерам (англ. worker - рабочий): при запуске приложения запускаются
несколько его копий, а балансировщик сервера распределяет запросы между ними.

В качестве менеджера инициализации дистрибутив использует systemd, поэтому
службу для запуска/остановки веб-сервера опишем согласно требованиям systemd.

Документация, к которой мы хотим привязать свое доменное имя, размещена
на https://readthedocs.org/ и сгенерирована с помощью системы автоматической
генерации документации `Sphinx <http://www.sphinx-doc.org/en/stable/>`_.

Для получения бесплатного SSL сертификата воспользуемся сервисом
`Let's Encrypt <https://letsencrypt.org/>`_. Сервис использует протокол
ACME (Automated Certificate Management Environment) для выдачи сертификатов.
LetsEncrypt предлагает `множество программных средств <https://letsencrypt.org/docs/client-options/>`_,
позволяющих получить сертификат через этот сервис. Самым менеджером сертификатов
показался `Acme.sh <https://github.com/Neilpang/acme.sh>`_ - реализация протокола
ACME на языке bash. Скрипт позволяет получить SSL сертификат на одно или
несколько доменных имен и дополнительно настроить службу cron для
переодической проверки и обновления полученных сертификатов.

Концепция
=========

Через сервис http://freenom.com/ зарегистрирован домен hlebushe.gq.
Есть желание предоставить доступ к веб-приложению при подключении к
https://hlebushe.gq/, а к документации через https://gnu-linux.hlebushe.gq/.

Сервис freenom имеет собственный сервер имен и позволяет настраивать поддомены
для своего доменного имени. Однако сервис не дает всех возможностей по тонкой
настройке DNS. К счастью, сервис позволяет зарегистрировать glue records
(адреса DNS серверов, управляющих доменной зоной) для своего домена
и провести самостоятельную настройку зоны на стороне своего сервера.
В перспективе это является более удобным решением, поскольку в этом случае
мы не привязаны к конкретному сервису.

Веб-приложению не обязательно знать, как к нему будет происходить доступ:
по HTTP или HTTPS. Вопросы безопасности можно решить уровнем выше: через
настройку виртуального хоста в Nginx. А доступ к самому приложению будет
осуществляться по принципу reverse proxy (обратный прокси). В моем случае
в роли обратного прокси сервера будет выступать nginx, а в качестве сервера
приложения - Gunicorn. Очевидно, доступ к приложению через Gunicorn должен
быть доступен только локально, а из внешней сети доступ к приложению будет
осуществляться через nginx.

Для настройки доступа к документации через свой домен есть оффициальное
`руководство <http://docs.readthedocs.io/en/latest/alternate_domains.html>`_.
Поскольку CNAME записи для доступа через протокол HTTPS не поддерживаются,
в руководстве предлагается настроить обратное проксирование.

Получать сертификаты будем на каждое доменное имя в отдельности, чтобы части
системы были независимы друг от друга.

Таким образом, вся система будет состоять из слабо связанных и заменяемых
частей::

    1. Внешний DNS, от которого требуется только перенаправлять запрос
    на наш DNS для получения доменного имени.
    2. Собственная DNS служба.
    3. Веб-сервер nginx, выполняющий обратное проксирование.
    4. Веб-сервер приложения, слушающий внутренний порт.
    5. Внешний сервер с размещенной документацией.
    6. Средство для управления сертификатами.

Настройка DNS
=============

Все команды выполняются от имени пользователя ``root``.

В первую очередь необходимо установить DNS службу на сервер::

    $ apt install bind9

После этого требуется описать файл зоны для нашего домена::

    $ cd /etc/bind/
    $ cp db.empty db.hlebushe.gq  # копия пустой зоны
    $ editor db.hlebushe.gq  # запуск редактора

Редактируем файл зоны::

    $ORIGIN hlebushe.gq.
    $TTL    86400

    @       IN      SOA     hlebushe.gq. root.hlebushe.gq. (
                                18          ; Serial
                                86400       ; Refresh
                                3600        ; Retry
                                2419200     ; Expire
                                86400       ; Minimum
                            )

    ; Сервера имен
                    NS      ns1.hlebushe.gq.
                    NS      ns2.hlebushe.gq.

    ; Адрес корня зоны
                    A       62.109.3.197

    ; Адреса серверов имен
    $TTL 86400
    ns1             A       62.109.3.197
    ns2             A       62.109.3.197

    ; Поддомены
    $TTL 600
    gnu-linux       CNAME   hlebushe.gq.

Дублирование адреса сервера имен доменной зоны необходимо по меньшей мере
по той причине, что сервис freenom не позволяет зарегистрировать
меньше двух собственных серверов имен. В этом случае это своеобразный хак.

Перезагружаем DNS службу::

    $ systemctl restart bind9

Пробуем получить адрес hlebushe.gq через локальный DNS сервер::

    $ dig @localhost hlebushe.gq
    ...
    ;; ANSWER SECTION:
    hlebushe.gq.            86400   IN      A       62.109.3.197

    ;; AUTHORITY SECTION:
    hlebushe.gq.            86400   IN      NS      ns2.hlebushe.gq.
    hlebushe.gq.            86400   IN      NS      ns1.hlebushe.gq.

    ;; ADDITIONAL SECTION:
    ns1.hlebushe.gq.        86400   IN      A       62.109.3.197
    ns2.hlebushe.gq.        86400   IN      A       62.109.3.197
    ...
    $ dig @localhost gnu-linux.hlebushe.gq
    ...
    ;; ANSWER SECTION:
    gnu-linux.hlebushe.gq.  60      IN      CNAME   hlebushe.gq.
    hlebushe.gq.            86400   IN      A       62.109.3.197

    ;; AUTHORITY SECTION:
    hlebushe.gq.            86400   IN      NS      ns1.hlebushe.gq.
    hlebushe.gq.            86400   IN      NS      ns2.hlebushe.gq.

    ;; ADDITIONAL SECTION:
    ns1.hlebushe.gq.        86400   IN      A       62.109.3.197
    ns2.hlebushe.gq.        86400   IN      A       62.109.3.197
    ...

Теперь зарегистрируем ns1 и ns2 в качестве сервера (серверов) имен нашей
доменной зоны. В панели управления доменом на сервисе freenom укажем
доменные имена ns1 и ns2.

.. image:: _static/img/k-gnu-linux/freenom-0.png

И зарегистрируем glue record для ns1.

.. image:: _static/img/k-gnu-linux/freenom-1.png

Аналогично регистрируется и ns2 (с тем же IP адресом).

Теперь с любого компьютера, подключенного к глобальной сети, можно получить
IP адреса по прописанным в файле зоны доменным именам (например, через DNS Google)::

    $ dig @8.8.8.8 hlebushe.gq
    ...
    ;; ANSWER SECTION:
    hlebushe.gq.		21599	IN	A	62.109.3.197
    ...
    $ dig @8.8.8.8 gnu-linux.hlebushe.gq
    ...
    ;; ANSWER SECTION:
    gnu-linux.hlebushe.gq.	59	IN	CNAME	hlebushe.gq.
    hlebushe.gq.		21599	IN	A	62.109.3.197
    ...

Настройка Nginx и получение сертификата
=======================================

Сперва установим Nginx::

    $ apt install nginx

Создадим файлы конфигураций виртуальных хостов и создадим ссылки на них
в sites-enabled::

    $ cd /etc/nginx/sites-available
    $ touch hlebushe.gq gnu-linux.hlebushe.gq
    $ ln -s $PWD/hlebushe.gq $PWD/gnu-linux.hlebushe.gq ../sites-enabled

Подготовимся к получению SSL сертификата. Acme.sh позволяет временно
переконфигурировать nginx для получения сертификатов. Но поскольку
я не доверяю скриптам производить какую-либо конфигурацию, делаю по-своему.
Заполним файл виртуального хоста
для hlebushe.gq::

    server {
        listen 80;
        server_name hlebushe.gq;
        location / {
            # Перенаправляем все подключения на https
            return 301 https://$host$request_uri;
        }
        location /.well-known/acme-challenge {
            # Разрешаем доступ по этому пути. Это необходимо для работы
            # протокола ACME, поскольку работает он через HTTP.
            # (Мы же не можем договорится по HTTPS с тем, кто дает сертификаты,
            #  если сертификата у нас еще нет)
            alias /var/www/hlebushe.gq/.well-known/acme-challenge;
            try_files $uri $uri/;
        }
    }

Аналогично для gnu-linux.hlebushe.gq::

    server {
        listen 80;
        server_name gnu-linux.hlebushe.gq;
        location / {
            return 301 https://$host$request_uri;
        }
        location /.well-known/acme-challenge {
            alias /var/www/gnu-linux.hlebushe.gq/.well-known/acme-challenge;
            try_files $uri $uri/;
        }
    }

Дополнительно необходимо создать указанные в конфиге директории::

    $ mkdir -p /var/www/hlebushe.gq/.well-known
    $ mkdir -p /var/www/gnu-linux.hlebushe.gq/.well-known

Настроим права доступа для пользователя Nginx::

    $ groupadd webusers
    $ useradd -r nginxuser
    $ usermod -aG webusers nginxuser
    $ # Предполагаем, что никто из webusers не будет писать в наши директории
    $ chown -R root:webusers \
        /var/www/hlebushe.gq \
        /var/www/gnu-linux.hlebushe.gq
    $ chmod -R u+rwX,g+rX,g-w,o-rwx \
        /var/www/hlebushe.gq \
        /var/www/gnu-linux-hlebushe.gq

И добавим (исправим) в начале конфигурационного файла /etc/nginx/nginx.conf строчку::

    user nginxuser;

Перезагрузим Nginx:

    $ systemctl restart nginx

Теперь у нас nginx работает от имени пользователя nginxuser, и есть
два виртуальных хоста на порте 80 (HTTP).

Получение SSL сертификатов
==========================

Получать сертификат будем с помощью Acme.sh. В репозитории
`Acme.sh на GitHub <https://github.com/Neilpang/acme.sh>`_ есть инструкция
по использованию скрипта. Рекоммендуется выполнять эти действия
под пользователем root. Сделаем то, что нам предлагается::

    $ cd ~
    $ wget -O -  https://get.acme.sh | sh
    $ DNAME=hlebushe.gq
    $ acme.sh --issue \
        -d $DNAME
        -w /var/www/$DNAME

Скрипт в течение нескольких секунд выполнит необходимые действия по
получению сертификата для указанного доменного имени. Аналогично сделаем для
второго доменного имени::

    $ DNAME=gnu-linux.hlebushe.gq
    $ acme.sh --issue \
        -d $DNAME
        -w /var/www/$DNAME

После этого сертификаты необходимо "установить" в директорию, откуда
их будет подхватывать Nginx. Создадим директорию, где будут храниться
сертификаты::

    $ mkdir -p /etc/sslcerts
    $ # И отдельную директорию для каждого домена
    $ mkdir /etc/sslcerts/hlebushe.gq
    $ mkdir /etc/sslcerts/gnu-linux.hlebushe.gq

Установим сертификаты в эти директории средствами Acme.sh::

    $ DNAME=hlebushe.gq
    $ acme.sh --install-cert \
        -d $DNAME \
        --key-file /etc/sslcerts/$DNAME/key.pem  \
        --fullchain-file /etc/sslcerts/$DNAME/cert.pem \
        --reloadcmd 'systemctl reload nginx'

И аналогично для второго домена::

    $ DNAME=gnu-linux.hlebushe.gq
    $ acme.sh --install-cert \
        -d $DNAME \
        --key-file /etc/sslcerts/$DNAME/key.pem  \
        --fullchain-file /etc/sslcerts/$DNAME/cert.pem \
        --reloadcmd 'systemctl reload nginx'

Настроим права доступа к файлам сертификата::

    $ # Предполагаем, что никто из webusers не будет
      # писать в директории с сертификатами,
    $ chown -R root:webusers \
        /etc/sslcerts/hlebushe.gq \
        /etc/sslcerts/gnu-linux.hlebushe.gq
    $ chmod -R u+rwX,g+rX,g-w,o-rwx \
        /etc/sslcerts/hlebushe.gq \
        /etc/sslcerts/gnu-linux.hlebushe.gq

Теперь для каждого доменного имени необходимо добавить виртуальные
хосты для порта 443 (порт HTTPS) в конфигурационных файлах Nginx.
И там же указать расположение файлов сертификата. Для hlebushe.gq::

    server {
        listen 443 ssl;
        server_name hlebushe.gq;

        if ($host != "hlebushe.gq") {
            return 403;
        }

        ssl_certificate /etc/sslcerts/hlebushe.gq/cert.pem;
        ssl_certificate_key /etc/sslcerts/hlebushe.gq/key.pem;
    }

И аналогично для gnu-linux.hlebushe.gq::

    server {
        listen 443 ssl;
        server_name gnu-linux.hlebushe.gq;

        if ($host != "gnu-linux.hlebushe.gq") {
            return 403;
        }

        ssl_certificate /etc/sslcerts/gnu-linux.hlebushe.gq/cert.pem;
        ssl_certificate_key /etc/sslcerts/gnu-linux.hlebushe.gq/key.pem;
    }

Перезапустим Nginx::

    $ systemctl restart nginx

Попробуем подключиться к http://hlebushe.gq/. Nginx перенаправляет нас
на https://hlebushe.gq и мы видим стандартную страницу приветствия Nginx.

.. image:: _static/img/k-gnu-linux/hi-nginx-with-https.png

Запуск веб-приложения
=====================

Поскольку приложение написано на Python, целесообразно его запускать
из-под виртуального окружения (virtualenv). Настройка виртуального окружения для
запуска приложения и непосредственного веб-сервера выходит за рамки данной
работы и рассмотрена не будет.

Полагаем, что имеется виртуальное окружение Python в директории
`/opt/hlebushe.gq` со всеми необходимыми приложению зависимостями
и веб-сервером Gunicorn. В `/opt/hlebushe.gq/hlebushe.gq` лежат
исходники приложения. В `/etc/django/apps/hlebushe.gq/gunicorn.py`
лежит конфигурационный файл для Gunicorn.

Создадим службу для запуска и остановки приложения через Gunicorn внутри
виртуального окружения.

    $ cd /etc/systemd/system
    $ editor gc-hlebushe.gq.service

Заполним файл службы::

    [Unit]
    Description=Gunicorn Server running hlebushe.gq Django WSGI app
    After=network.target

    [Service]
    PIDFile=/run/hlebushe.gq/gc-pid
    User=dj
    Group=webusers
    ExecStart=/bin/bash -c '\
        cd /opt/hlebushe.gq; \
        source bin/activate; \
        cd hlebushe.gq; \
        exec gunicorn \
            -c /etc/django/apps/hlebushe.gq/gunicorn.py
            hleb.wsgi \
    '
    ExecReload=/bin/kill -s HUP $MAINPID
    ExecStop=/bin/kill -s TERM $MAINPID

    [Install]
    WantedBy=multi-user.target

Создадим директорию для PID файла и установим права доступа пользователю dj::

    $ mkdir -p /run/hlebushe.gq
    $ chown -R dj:webusers
    $ chmod -R u+rwX,g+rX,g-w,o-rwx /run/hlebushe.gq

Запускаем новый сервис::

    $ systemctl daemon-reload
    $ systemctl enable --now gc-hlebushe.gq.service
    $ systemctl status gc-hlebushe.gq.service
    ● gc-hlebushe.gq.service - Gunicorn Server which runs hlebushe.gq Django site
       Loaded: loaded (/etc/systemd/system/gc-hlebushe.gq.service; enabled; vendor preset: enabled)
       Active: active (running) since Tue 2017-12-19 12:10:22 EST; 3s ago
      Process: 13257 ExecStop=/bin/kill -s TERM $MAINPID (code=exited, status=0/SUCCESS)
     Main PID: 13260 (gunicorn)
       CGroup: /system.slice/gc-hlebushe.gq.service
               ├─13260 /opt/hlebushe.gq/bin/python3 /opt/hlebushe.gq/bin/gunicorn ...
               ├─13266 /opt/hlebushe.gq/bin/python3 /opt/hlebushe.gq/bin/gunicorn ...
               └─13268 /opt/hlebushe.gq/bin/python3 /opt/hlebushe.gq/bin/gunicorn ...

Видим, что Gunicorn работает под управлением Python3 из виртуального окружения.
Ровно как и исполняемый файл Gunicorn взят из виртуального окружения.

В конфигурационном файле для сервера Gunicorn прописан адрес 127.0.0.1 и порт 12000.
То есть, пока есть только локальный доступ к приложению.
При попытке постучаться на главную сраничку, сервер выводит html заглвной страницы::

    $ wget -nv -O - http://127.0.0.1:12000/ | head -n 15
    <!doctype html>
    <html>
      <head>

        <meta content="text/html; charset=UTF-8" http-equiv="content-type"></meta>
        <title>

            Хлебуше.к

        </title>
        <link type="text/css" rel="stylesheet"
              href="https://fonts.googleapis.com/css?family=Roboto+Condensed:400,...
        <link type="text/css" rel="stylesheet"
              href="/static/css/normalize.css"/>
        <link type="text/css" rel="stylesheet"
    2017-12-19 12:28:07 URL:http://127.0.0.1:12000/ [3072/3072] -> "-" [1]

Добавим обратное проксирование в Nginx на локально доступный порт.
Для этого необходимо отредактировать файл с виртуальным хостом для hlebushe.gq
(конфиг /etc/nginx/sites-available/hlebushe.gq). Добавим следующее::

    server {
        listen 443 ssl;
        server_name hlebushe.gq;
        ...

        # Обратное проксирование на Gunicorn
        location @gunicorn {
            proxy_pass http://127.0.0.1:12000;
            proxy_set_header Host $host;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_redirect off;
        }

        # Предлагаем nginx стучаться в Gunicorn при всех запросах
        location / {
            try_files @gunicorn @gunicorn;
        }

        # Но статику отдаем через nginx, чтобы не нагружать Gunicorn
        location /static/ {
            alias /var/www/hlebushe.gq/static/;
            try_files $uri $uri/;
        }
    }

Опционально можно добавить пути к логам и Keep-Alive таймаут. Конечный
вариант файла выглядит вот так::

    server {
        listen 80;
        server_name hlebushe.gq;
        location / {
            return 301 https://$host$request_uri;
        }
        location /.well-known/acme-challenge {
            alias /var/www/webim.hlebushe.gq/.well-known/acme-challenge;
            try_files $uri $uri/;
        }
    }

    server {
        listen 443 ssl;
        server_name hlebushe.gq;

        if ($host != "hlebushe.gq") {
            return 403;
        }

        ssl_certificate /etc/sslcerts/hlebushe.gq/cert.pem;
        ssl_certificate_key /etc/sslcerts/hlebushe.gq/key.pem;

        error_log /var/log/nginx/hlebushe.gq.error.log;
        access_log /var/log/nginx/hlebushe.gq.log;

        keepalive_timeout 5;

        location @gunicorn {
            proxy_pass http://127.0.0.1:12000;
            proxy_set_header Host $host;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_redirect off;
        }

        location / {
            try_files @gunicorn @gunicorn;
        }

        location /static/ {
            alias /var/www/hlebushe.gq/static/;
            try_files $uri $uri/;
        }
    }

Перезагружаем Nginx. Пробуем зайти на https://hlebushe.gq/. Видим стартовую страницу.

.. image:: _static/img/k-gnu-linux/hlebushe.gq-index.png

Проксирование на документацию
=============================

Аналогично дописываем виртуальный хост для порта 443 в
/etc/nginx/sites-available/gnu-linux.hlebushe.gq, указывая файлы с сертификатами::

    server {
        listen 443 ssl;
        server_name gnu-linux.hlebushe.gq;

        ssl_certificate /etc/sslcerts/gnu-linux.hlebushe.gq/cert.pem;
        ssl_certificate_key /etc/sslcerts/gnu-linux.hlebushe.gq/key.pem;

    }

Предварительно необходимо добавить свой домен в настройках своего проекта
на readthedocs.io (Admin > Domains).

.. image:: _static/img/k-gnu-linux/rtd-settings.png

Согласно `руководству <http://docs.readthedocs.io/en/latest/alternate_domains.html>`_
параметр proxy_pass должен содержать ссылку на readthedocs.io, по которой
раздается документация. Параметр proxy_set_header X-RTD-SLUG должен
содержать т. н. slug проекта. Он соответствует имени проекта, если последний
не был переименован.

Дописываем настройки для обратного проксирования на нашу документацию
в /etc/nginx/sites-available/gnu-linux.hlebushe.gq::

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

Итоговый вариант /etc/nginx/sites-available/gnu-linux.hlebushe.gq::

    server {
        listen 80;
        server_name gnu-linux.hlebushe.gq;

        location / {
            return 301 https://$host$request_uri;
        }
        location /.well-known/acme-challenge {
            alias /var/www/gnu-linux.hlebushe.gq/.well-known/acme-challenge;
            try_files $uri $uri/;
        }
    }

    server {
        listen 443 ssl;
        server_name gnu-linux.hlebushe.gq;

        if ($host != "gnu-linux.hlebushe.gq") {
            return 403;
        }

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

Выполняем::

    systemctl restart nginx

Заходим на https://gnu-linux.hlebushe.gq. Видим стартовую страницу с документацией.

.. image:: _static/img/k-gnu-linux/rtd-index.png

Итоги
=====

Цель достигнута. В процессе работы была рассмотрена методика конфигурирования
DNS сервера для своей доменной зоны, конфигурирования веб-сервера Nginx в
качестве обратного прокси на свое веб-приложение и документацию, размещенную
на readthedocs.org. Создание службы для запуска и остановки работы
внутреннего сервера WSGI приложения.
