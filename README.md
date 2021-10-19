Проект для подключения бризера Tion O2 к умному дому через протокол ZigBee

Отлаживалось на бризерах выпущенных в октябре 2020 года. На экземплярах выпущенных раньше или позже может использоваться другой дисплей или другая прошивка.

В бризере в качестве контроллера ЖК дисплея используется микросхема HOLTEK HT1622.
Основная идея заключается в том, что мы цепляемся паралельно входу этой микросхемы и снифим данные, отправляемые в неё.
Для управления замыкаем выводы кнопок панели управления бризера имитируя, таким образом, нажатия на них.

Ядром проекта является микроконтроллер ATmega328P.
Для работы по протоколу ZigBee используется модуль E18-MS1-PCB. Так же пробовал модуль E18-MS1PA1-PCB, но из-за встроенного усилителя он слишком много потребляет, и его не вытягивает встроенный блок питания бризера.

При изменениях температуры или режима работы бризера, а также примерно каждые 20 минут на координатор отправляется текущий статус бризера.
Посылка представляет из JSON вида: {"PWR":"1","APW":"1","OUT":"5","SPD":"3","HTE":"1","TMS":"0","HTT":"18"}
Параметры:
PWR - работает бризер или в режиме standbuy (0 - standbuy, 1 - работает)
APW - включён ли режим автоматического включения после сбоя питания. Если 1, то после подачи питания, если на момент сбоя бризер был включён, то бризер включится.
OUT - температура входящего воздуха
SPD - скорость работы вентилятора (от 1 до 4)
HTE - включён ли подогрев воздуха
HTT - до какой температуры подогревается входящий воздух
FLT - осталось дней до замены фильтров
MNT - минимально допустимая температура входящего воздуха
TIM - текущее время (формат ЧЧММ)
TMS - включён ли таймер включения/выключения
TME - время включения (формат ЧЧММ)
TMD - время выключения (формат ЧЧММ)
ERROR - в случае, если бризер отобразил ошибку, то будет отправлен только этот параметр, в значении будет строка с ошибкой. Так же, в случае, если не удалось распознать данные на дисплее, то в значении будет пусто.

Для управления необходимо отправить строку, содержащую одну или несколько команд.
Строка состоит из названия параметра, разделителя, значения параметра. Разделителем является двоеточие ":".
Можно отправить разом несколько параметров. В этом случае параметры должны отделаться друг от друга запятой ","
Оканчиваться строка должна символом возврата каретки (код символа 0x0D).
Максимальная длинна строки 127 символов.
Параметры будут применяться в том же порядке, что и в отправленной команде.
Если встречается ошибочный параметр, то дальнейший разбор строки прекращается.

Пример строки: PWR:1,SPD:2,HTE:1,HTT:17\r

Сами параметры аналогичны тем, что перечислены в статусе, за исключением параметра FLT - изменение срока замены фильтров я не реализовывал, т.к. не увидел необходимости в этом.
Так же, добавлена дополнительная команда EXS. Значение можно выставить любое число до 4 знаков, например 1.
При получении этой команды контроллер проходит по всем настройкам бризера и отправляет их координатору ZigBee сети. Это нужно, чтобы получить текущие значения таймеров, текущего времени, срока службы фильтров и минимально допустимой температуры.

На стороне координатора я использую проект zigbee2mqtt. В нём статус можно получить по адресу zigbee2mqtt/device_id/action
Строка с командами же отправляется по адресу zigbee2mqtt/device_id/set/action содержимое PWR:1,SPD:2,HTE:1,HTT:17

Каталоги:
arduino - проект в Arduino IDE версии 1.8.16 - сдесь код для контроллера ATmega328P
kicad - проект в kiCAD версии 5.0.2 - схема платы, печатная плата и готовые герберы.
CC2530 - прошивки для ZigBee модулей
TION-O2_ZIGBEE.js - файл описания устройства для zigbee2mqtt. Его надо положить в каталог data проекта zigbee2mqtt и прописать в файле configuration.yaml:

    external_converters:
      - TION-O2_ZIGBEE.js

Прошивка для ZigBee модуля создана с помощью контруктора прошивок с сайта https://ptvo.info
Вдохновлялся проектом https://hackaday.io/project/168959-let-me-control-you-hitachi-air-conditioner