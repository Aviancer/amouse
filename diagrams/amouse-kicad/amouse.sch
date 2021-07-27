EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Anachro Mouse, USB to Serial Mouse Adapter"
Date "2021-07-27"
Rev "1"
Comp "github.com/Aviancer/amouse"
Comment1 "twitter.com/skyvian"
Comment2 "Licensed under TAPR Open Hardware License v1.0 (www.tapr.org/OHL)"
Comment3 "Copyright (C) 2021 Aviancer"
Comment4 ""
$EndDescr
$Comp
L Connector:USB_A J1
U 1 1 60DBFF4A
P 1850 3600
F 0 "J1" H 1850 4100 50  0000 C CNN
F 1 "USB_A" H 1850 4000 50  0000 C CNN
F 2 "" H 2000 3550 50  0001 C CNN
F 3 " ~" H 2000 3550 50  0001 C CNN
	1    1850 3600
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60DC180A
P 1850 4000
F 0 "#PWR?" H 1850 3750 50  0001 C CNN
F 1 "GND" H 1855 3827 50  0000 C CNN
F 2 "" H 1850 4000 50  0001 C CNN
F 3 "" H 1850 4000 50  0001 C CNN
	1    1850 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	1750 4000 1850 4000
Connection ~ 1850 4000
Text GLabel 2150 3600 2    50   BiDi ~ 0
USB_D+
Text GLabel 2150 3700 2    50   BiDi ~ 0
USB_D-
$Comp
L power:GND #PWR?
U 1 1 60DC3812
P 9800 4500
F 0 "#PWR?" H 9800 4250 50  0001 C CNN
F 1 "GND" H 9805 4327 50  0000 C CNN
F 2 "" H 9800 4500 50  0001 C CNN
F 3 "" H 9800 4500 50  0001 C CNN
	1    9800 4500
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR?
U 1 1 60DD9A8C
P 5200 2800
F 0 "#PWR?" H 5200 2650 50  0001 C CNN
F 1 "+5V" V 5215 2928 50  0000 L CNN
F 2 "" H 5200 2800 50  0001 C CNN
F 3 "" H 5200 2800 50  0001 C CNN
	1    5200 2800
	1    0    0    -1  
$EndComp
Text GLabel 9100 3900 2    50   Output ~ 0
RS232_TX
Text GLabel 9050 4100 2    50   Input ~ 0
RS232_RX
Text GLabel 3500 3000 0    50   Input ~ 0
TTL_RX
Text GLabel 3500 2900 0    50   Output ~ 0
TTL_TX
$Comp
L Connector:DB9_Male_MountingHoles J2
U 1 1 60DB81E1
P 9800 3900
F 0 "J2" H 9980 3809 50  0000 L CNN
F 1 "DB9_Male_MountingHoles" H 9350 2950 50  0000 L CNN
F 2 "" H 9800 3900 50  0001 C CNN
F 3 " ~" H 9800 3900 50  0001 C CNN
	1    9800 3900
	1    0    0    -1  
$EndComp
Text GLabel 9500 3800 0    50   Output ~ 0
RS232_CTS
NoConn ~ 9500 4300
NoConn ~ 9500 4200
NoConn ~ 9500 4000
NoConn ~ 9500 3700
NoConn ~ 9500 3600
Text GLabel 3500 3200 0    50   Input ~ 0
TTL_CTS
NoConn ~ 3500 3100
NoConn ~ 3500 3300
NoConn ~ 3500 3400
$Comp
L power:GND #PWR?
U 1 1 60DC5B9C
P 5200 3100
F 0 "#PWR?" H 5200 2850 50  0001 C CNN
F 1 "GND" V 5200 2900 50  0000 C CNN
F 2 "" H 5200 3100 50  0001 C CNN
F 3 "" H 5200 3100 50  0001 C CNN
	1    5200 3100
	1    0    0    -1  
$EndComp
NoConn ~ 3500 3500
NoConn ~ 3500 3600
NoConn ~ 3500 3700
NoConn ~ 3500 3800
NoConn ~ 3500 3900
NoConn ~ 3500 4000
NoConn ~ 3500 4100
NoConn ~ 3500 4200
NoConn ~ 3500 4300
NoConn ~ 3500 4400
NoConn ~ 3500 4500
NoConn ~ 3500 4600
NoConn ~ 3500 4700
NoConn ~ 3500 4800
NoConn ~ 4900 4800
NoConn ~ 4900 4700
NoConn ~ 4900 4600
NoConn ~ 4900 4500
NoConn ~ 4100 5000
NoConn ~ 4200 5000
NoConn ~ 4300 5000
NoConn ~ 4900 4400
NoConn ~ 4900 4300
NoConn ~ 4900 4200
NoConn ~ 4900 4100
NoConn ~ 4900 4000
NoConn ~ 4900 3900
NoConn ~ 4900 3800
NoConn ~ 4900 3700
NoConn ~ 4900 3600
NoConn ~ 4900 3500
NoConn ~ 4900 3400
NoConn ~ 4900 3300
NoConn ~ 4900 3200
$Comp
L Raspberry_Pico:Pico U1
U 1 1 60DD26B6
P 4200 3850
F 0 "U1" H 4200 5700 50  0000 C CNN
F 1 "Pico" H 4200 5600 50  0000 C CNN
F 2 "RPi_Pico:RPi_Pico_SMD_TH" V 4200 3850 50  0001 C CNN
F 3 "" H 4200 3850 50  0001 C CNN
	1    4200 3850
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x01_Female TP3
U 1 1 60DE5CF6
P 4300 2900
F 0 "TP3" V 4146 2948 50  0001 L CNN
F 1 "TP3" H 3850 3000 50  0000 L CNN
F 2 "" H 4300 2900 50  0001 C CNN
F 3 "~" H 4300 2900 50  0001 C CNN
	1    4300 2900
	0    1    1    0   
$EndComp
Text GLabel 4100 2700 1    50   BiDi ~ 0
USB_D-
Text GLabel 4300 2700 1    50   BiDi ~ 0
USB_D+
$Comp
L Connector:Conn_01x01_Female TP2
U 1 1 60DE6353
P 4100 2900
F 0 "TP2" V 3946 2948 50  0001 L CNN
F 1 "TP2" H 3650 2800 50  0000 L CNN
F 2 "" H 4100 2900 50  0001 C CNN
F 3 "~" H 4100 2900 50  0001 C CNN
	1    4100 2900
	0    1    1    0   
$EndComp
Wire Wire Line
	2150 3600 2650 3600
Wire Wire Line
	2650 3600 2650 2200
Wire Wire Line
	2650 2200 4300 2200
Wire Wire Line
	4300 2200 4300 2700
Wire Wire Line
	2150 3700 2750 3700
Wire Wire Line
	2750 3700 2750 2300
Wire Wire Line
	2750 2300 4100 2300
Wire Wire Line
	4100 2300 4100 2700
Wire Wire Line
	3500 2900 2950 2900
Wire Wire Line
	2950 2900 2950 5500
Wire Wire Line
	3500 3000 3050 3000
Wire Wire Line
	3050 3000 3050 5400
Wire Wire Line
	3500 3200 3150 3200
Wire Wire Line
	3150 3200 3150 5300
Wire Wire Line
	5300 4500 5300 5300
Wire Wire Line
	5300 5300 3150 5300
Wire Wire Line
	3050 5400 5400 5400
Wire Wire Line
	5400 5400 5400 4300
Wire Wire Line
	2950 5500 5500 5500
Wire Wire Line
	5500 5500 5500 3900
Wire Wire Line
	8850 4500 8850 3800
Wire Wire Line
	8850 3800 9500 3800
Wire Wire Line
	8050 4500 8850 4500
Wire Wire Line
	8050 3900 9500 3900
Wire Wire Line
	5500 3900 6450 3900
Wire Wire Line
	5400 4300 6450 4300
Wire Wire Line
	6450 4500 5300 4500
$Comp
L Interface_UART:MAX3232 U2
U 1 1 60DC4834
P 7250 3800
F 0 "U2" H 7250 5400 50  0000 C CNN
F 1 "MAX3232" H 7250 5300 50  0000 C CNN
F 2 "" H 7300 2750 50  0001 L CNN
F 3 "https://datasheets.maximintegrated.com/en/ds/MAX3222-MAX3241.pdf" H 7250 3900 50  0001 C CNN
	1    7250 3800
	1    0    0    -1  
$EndComp
Text Notes 8350 3600 0    50   ~ 0
0.1 uF
Text Notes 8350 3300 0    50   ~ 0
0.1 uF
Text Notes 8350 3000 0    50   ~ 0
0.1 uF
Text Notes 6000 3050 0    50   ~ 0
0.1 uF
Text GLabel 6450 4500 0    50   Output ~ 0
TTL_CTS
Text GLabel 8050 4500 2    50   Input ~ 0
RS232_CTS
NoConn ~ 8050 4100
$Comp
L power:GND #PWR?
U 1 1 60DE0C0B
P 8650 3500
F 0 "#PWR?" H 8650 3250 50  0001 C CNN
F 1 "GND" V 8655 3372 50  0000 R CNN
F 2 "" H 8650 3500 50  0001 C CNN
F 3 "" H 8650 3500 50  0001 C CNN
	1    8650 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	6350 3200 6350 3150
Wire Wire Line
	6450 3200 6350 3200
Wire Wire Line
	6350 2900 6350 2950
Wire Wire Line
	6450 2900 6350 2900
Wire Wire Line
	8150 3200 8150 3150
Wire Wire Line
	8050 3200 8150 3200
Wire Wire Line
	8150 2900 8150 2950
Wire Wire Line
	8050 2900 8150 2900
$Comp
L Device:CP_Small C1
U 1 1 60DCDCB9
P 6350 3050
F 0 "C1" H 6438 3096 50  0000 L CNN
F 1 "CP_Small" H 6438 3005 50  0000 L CNN
F 2 "" H 6350 3050 50  0001 C CNN
F 3 "~" H 6350 3050 50  0001 C CNN
	1    6350 3050
	1    0    0    -1  
$EndComp
$Comp
L Device:CP_Small C2
U 1 1 60DD17D0
P 8150 3050
F 0 "C2" H 8238 3096 50  0000 L CNN
F 1 "CP_Small" H 8238 3005 50  0000 L CNN
F 2 "" H 8150 3050 50  0001 C CNN
F 3 "~" H 8150 3050 50  0001 C CNN
	1    8150 3050
	1    0    0    -1  
$EndComp
NoConn ~ 6450 4100
Text GLabel 6450 3900 0    50   Input ~ 0
TTL_TX
Text GLabel 6450 4300 0    50   Output ~ 0
TTL_RX
Text GLabel 8050 4300 2    50   Input ~ 0
RS232_RX
Text GLabel 8050 3900 2    50   Output ~ 0
RS232_TX
$Comp
L Device:CP_Small C4
U 1 1 60DD1E84
P 8150 3700
F 0 "C4" V 8200 3800 50  0000 C CNN
F 1 "CP_Small" V 8016 3700 50  0000 C CNN
F 2 "" H 8150 3700 50  0001 C CNN
F 3 "~" H 8150 3700 50  0001 C CNN
	1    8150 3700
	0    1    1    0   
$EndComp
$Comp
L Device:CP_Small C3
U 1 1 60DCF2E8
P 8150 3400
F 0 "C3" V 8200 3300 50  0000 C CNN
F 1 "CP_Small" V 8284 3400 50  0000 C CNN
F 2 "" H 8150 3400 50  0001 C CNN
F 3 "~" H 8150 3400 50  0001 C CNN
	1    8150 3400
	0    -1   -1   0   
$EndComp
$Comp
L power:+5V #PWR?
U 1 1 60DCBE28
P 7250 2600
F 0 "#PWR?" H 7250 2450 50  0001 C CNN
F 1 "+5V" H 7265 2773 50  0000 C CNN
F 2 "" H 7250 2600 50  0001 C CNN
F 3 "" H 7250 2600 50  0001 C CNN
	1    7250 2600
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60DCAC48
P 7250 5000
F 0 "#PWR?" H 7250 4750 50  0001 C CNN
F 1 "GND" H 7255 4827 50  0000 C CNN
F 2 "" H 7250 5000 50  0001 C CNN
F 3 "" H 7250 5000 50  0001 C CNN
	1    7250 5000
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 3100 5200 3100
Wire Wire Line
	8250 3700 8250 3400
Wire Wire Line
	8250 3400 8650 3400
Connection ~ 8250 3400
Wire Wire Line
	8650 3400 8650 3500
Wire Wire Line
	4900 2900 5200 2900
Wire Wire Line
	5200 2900 5200 2800
Wire Wire Line
	4900 3000 5200 3000
Wire Wire Line
	5200 3000 5200 2900
Connection ~ 5200 2900
$Comp
L power:+5V #PWR?
U 1 1 60DC08DF
P 2150 3250
F 0 "#PWR?" H 2150 3100 50  0001 C CNN
F 1 "+5V" V 2165 3378 50  0000 L CNN
F 2 "" H 2150 3250 50  0001 C CNN
F 3 "" H 2150 3250 50  0001 C CNN
	1    2150 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 3400 2150 3250
Wire Wire Line
	8050 4300 8750 4300
Wire Wire Line
	8750 4300 8750 4100
Wire Wire Line
	8750 4100 9500 4100
$Comp
L power:GND #PWR?
U 1 1 60DC3ECA
P 8850 3500
F 0 "#PWR?" H 8850 3250 50  0001 C CNN
F 1 "GND" V 8855 3372 50  0000 R CNN
F 2 "" H 8850 3500 50  0001 C CNN
F 3 "" H 8850 3500 50  0001 C CNN
	1    8850 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	8850 3500 9500 3500
$EndSCHEMATC
