#! /bin/bash

# Пуск сервера
lab6-server &
sleep 1

# Отправляем сообщеньки
lab6-client
lab6-client "Hello, World!"
lab6-client `perl -e 'print "a" x 300'`
lab6-client "Сообщение"
lab6-client "操作系统"
lab6-client "Пока!"
lab6-client "Ты тут?"
