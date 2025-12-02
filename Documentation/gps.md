Sample Boot Sequence
```
$GPTXT,01,01,02,MA=CASIC*27
$GPTXT,01,01,02,HW=ATGM332D,0009111648291*12
$GPTXT,01,01,02,IC=AT6558-5N-71-0C510800,J9K11CL-A4-013062*26
$GPTXT,01,01,02,SW=URANUS5,V5.2.1.0*1D
$GPTXT,01,01,02,TB=2019-08-12,10:01:41*4E
$GPTXT,01,01,02,MO=GBR*25
$GPTXT,01,01,02,BS=SOC_BootLoader,V6.2.0.2*34
$GPTXT,01,01,02,FI=00EF4014*7D
```

Sample 1s Push, No Satellites in view
```
$GNGGA,,,,,,0,00,25.5,,,,,,*64
$GNGLL,,,,,,V,M*79
$GPGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*02
$BDGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*13
$GLGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*1E
$GPGSV,1,1,00*79
$BDGSV,1,1,00*68
$GLGSV,1,1,00*65
$GNRMC,,V,,,,,,,,,,M*4E
$GNVTG,,,,,,,,,M*2D
$GNZDA,,,,,,*56
$GPTXT,01,01,01,ANTENNA OK*35
```

Sample push, few satellites in view, only time available (85 seconds, Purdue Campus, Near West-Facing Window)
```
$GNGGA,224452.000,,,,,0,00,25.5,,,,,,*7D
$GNGLL,,,,,224452.000,V,M*60
$GPGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*02
$BDGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*13
$GLGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*1E
$GPGSV,1,1,04,13,,,27,17,,,32,19,,,24,22,,,31*71
$BDGSV,1,1,00*68
$GLGSV,1,1,00*65
$GNRMC,224452.000,V,,,,,,,,,,M*57
$GNVTG,,,,,,,,,M*2D
$GNZDA,224452.000,,,,,*4F
$GPTXT,01,01,01,ANTENNA OK*35
```

Sample push, many satellites in view, date and time available (216 seconds, Purdue Campus, Near West-Facing Window)
```
$GNGGA,062157.000,,,,,0,00,4.5,,,,,,*4E
$GNGLL,,,,,062157.000,V,N*63
$GPGSA,A,1,,,,,,,,,,,,,7.0,4.5,5.3*30
$BDGSA,A,1,,,,,,,,,,,,,7.0,4.5,5.3*21
$GLGSA,A,1,,,,,,,,,,,,,7.0,4.5,5.3*2C
$GPGSV,3,1,09,05,83,243,,06,08,086,,12,21,206,16,13,21,148,*76
$GPGSV,3,2,09,18,05,278,,20,60,045,,21,61,050,,25,29,243,19*75
$GPGSV,3,3,09,29,52,309,23*47
$BDGSV,1,1,01,34,72,329,*53
$GLGSV,1,1,03,66,47,158,,65,57,065,,88,53,068,*53
$GNRMC,062157.000,V,,,,,,,251125,,,N*54
$GNVTG,,,,,,,,,N*2E
$GNZDA,062157.000,25,11,2025,00,00*4D
$GPTXT,01,01,01,ANTENNA OK*35
```