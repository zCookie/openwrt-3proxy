------------------------------ KOI8-R ------------------------------------
  ���� ����� �������� ����� CGI c������� � �������� ��� ��������� 
���������� ������ ������������� ������ ������� "3proxy", ����������� �������
���� �������������� � ODBC ���������(����), ����� Web ���������.                      

stat.awk - �������� CGI ������ (��� ��� ����������� ��� Win9X/2000 ���������� 
           ��������� awk.exe ,� linux/freebsd ��� ��� ������� ������ � ��������
           �� ���������).    
isqlodbc - ��������� ��� ���������� SQL �������� � ����� ODBC 
           (���������� �� stat.awk). ������������� gcc � �������� ��� � 
           win9X/2000 ��� � � linux/freebsd. (��� �� ����� 
           �������������� ���������� �� stat.awk ��� ��������� 
           ���������..)
log.sql  - SQL ������ �������� ���� ��� ���� �������.           
awk.exe  - awk �������������  ��� Win9X/2000.  

                        ��������� �������� ���������� .

��� ������ ��� �����������:
1) ����� http ������ ������������� CGI
2) odbc �������� (��� win32 ) ��� iodbc �������� (��� unix)
   ����� ���� ������ �������� : sqlite, mysql, postgress ��� ����� ������ 
   ������� ODBC ��������.(��� ����������� iODBC ��� linux/freebsd �������� �
   ����� iodbc.txt �  �������� /doc/ru ������ 3proxy.)

 ��� ��������� N1:
������� ���� ������ � DSN ��� �������� ����. ( � ����� ������ DSN ����� 
���������� "sqlite".) ����� �������� ������ log.sql ������� ����������� 
������� � �������:

isqlodbc sqlite < log.sql

 ��� ��������� N2:
������������� DSN � ������ ������� � ����� � ����� 3proxy.cfg ���������� ����:
-----------
# create table log (
#    ldate date,
#    ltime time,
#    username char (30),
#    userip char (16),
#    bytein integer (10),
#    byteout integer (10),
#    service char (8),
#    host char(255),
#    hostport integer (10),
#    url char (255)
#   );

log &sqlite
logformat "Linsert into log values ('%Y-%m-%d','%H:%M:%S','%U','%C','%I','%O','%N','%n','%r','%T');"
-----------

 ��� ��������� N3:
�������� ����� isqlodbc � stat.awk � ������� � CGI ��������� http ������� 
� ������ � stat.awk ���� ������ � DSN �� ���� �������� , ��������:
isql="./isqlodbc.exe sqlite " 

 ��� ��������� N4:
������� ������� ������ �� web �������� , �������� 

http://localhost/cgi/stat.awk?

------------------------------ KOI8-R ------------------------------------