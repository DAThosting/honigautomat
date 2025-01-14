
what do you have to check and configure in the arduino sketch:

Please check & update before you upload the sketch to your arduino.
youtube channel containing a set of videos about this vendor machine:
https://youtube.com/playlist?list=PLrYKRosgwf_19UN1uPkxFnaKNIluaASAk

#********************************************************************************#
are you interested in cashless payment (credit card or apple pay etc.) or 
in payment with bill (Zahlung mit Geldscheinen)? Please send a mail.
honigautmat@gmx.de
#********************************************************************************#

#################################################################################
Depending on selected board (e.g. UNO or Mega 2560) and number of buttons=selector and compartments the pin definition must be adapted as follows.
Please ensure that the cable connectors are established accordingly.
//uno
//const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0) UNO
// when you want to user 2 * 5 compartments with UNO it is not possible to have power-off features at the same time because of lack of pins. In this case I suggest
// to run 1 row with 5 and one row with 4 compartements; use the free PIN to steer the power off relais; as an alternative you can consider to use PIN 1 (which is not recommended because of issues with serial-IOs); this may be possible 
// if you set all lines dealing with "Serial..." to "// serial..."
//mega
//const int selector[15] = {A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // input pins for selector buttons MEGA
const int selector[5] = {A1, A2, A3, A4, A5}; // input pins for selector buttons MEGA 5 buttons and 5/10 or 15 boxes


for Uno with 5 compartments you can use more features than  having 10 compartments; 
to activate search for "refillbutton", "homematic" and "poweroff"; remove "//" in these areas


##################################################################################
Following row defines the price for each compartment. In this example it is 5€; Having 5 Buttons at the vendor machine even with 10 or 15 compartments the number of price is fix: 5
In dieser Zeile kann man vorgeben, welcher Preis für die Fächer vorbelegt werden soll (in diesem Beispiel 5€; es müssen immmer 5 Werte sein, auch wenn man 10 Fächer hat):

int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default 500ct (eeprom currently not used)

How many compartments/rows are available?
Wieviele Fächer je Reihe werden angeschlossen? 

bei 5 Fächern / 5 compartments:
int conveyorItems[5] = {1, 1, 1, 1, 1};
const int maxrow = 1;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct

bei 10 Fächern / 10 compartments:
int conveyorItems[5] = {2, 2, 2, 2, 2};
const int maxrow = 2;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct


bei 15 Fächern / 15 compartments:
int conveyorItems[5] = {3, 3, 3, 3, 3};
const int maxrow = 3;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct

If you want to run the system with a button for each compartment number of rows should be set to 1
##################################################################################
release 3

please consider that for UNO the pin assignment is now different than before!
**********************************************************************************
new feature with release 3 - for UNO 
check temperature with help of DS18B20 and turn cooling on/off 
the usage is recommended in summer time (this feauture is on and power-save disabled when you connect pin "summer_pin" to GND).
for winter disconnect this again.

Temperature sensor DS18B20
RED +5V
BLACK GND (5V)
YELLOW connect to pin "tempsensor" and via 4.7k ohm resistor with +5V

##################################################################################
NV10 - payment with bill now available for Mega 2560

DIPs on NV10:
1 LOW
2 HIGH (this has been changed recently!)
3 LOW
4 HIGH

DIP2 on HIGH to get 4 pulses instead of 1 because in some installations compartment opening impacted the pulse interrupt. Now all pulses less 4 are ignored.


please note that electronic payment with Najax Onyx is not yet ready but dev started! (beta)

int nv10_act = 13;
connect this pin to GND to activate feature

int nv10_ch1 = 9;   
int nv10_ch2 = 10;
connect to PIN 5 + 6 of NV10 to allow 5€ / 10€ (steering via sketch)

NV10 PINs:
1 connect to interrupt pin (arduino) and via 10kOhm to +5V
5 see above
6 see above
15 +12V (better turn on/off via coin acceptor relais "coin power") 
16 GND 12V

##################################################################################
new feature with release 2 - powersave:
after reaching idlePeriod relais "poweroffrelais" cuts the power; wakeup with poweron button (you need a separate self holding relais to keep power). See fritzing-documentation
This feature is active if powersave is set = 1 (otherwhise 0);
##################################################################################
new feature with release 2 - write/read eeprom
current price and remaining products in compartments per button-column will be stored in eeprom and is available after system restart. 
existing credit is deleted when restarting system
To force an eeprom update in case of new structure (e.g. additional set compartments to be activated)
##################################################################################
new feature with release 2 - reset credit after cutofftime = idle threshold
remaining credit will be deleted after a while
##################################################################################
new feature with release 3 - Jan 2023:
it is possible to disable the coin acceptor when all products are sold out
(mega only): connect PIN 17 with a free relais and lead power +12 for coin acceptor
through this relais (function not yet visible in fritzing drawing)

NV10 - bill acceptor now supported / der Geldscheinleser NV10 wird jetzt unterstützt

Payment cashless with Nayax Onyx (you need additional hardware and a contract with
Nayax for reconciliation/bank accounting; monthly fee & percentage to be paid.
Zahlung per Kreditkarte mit Nayax Onyx (in Testphase) wird jetzt unterstützt. 
Dafür wird zusätzliche Hardware sowie ein Supportvertrag mit Nayax benötigt. Dafür
fallen (geringe) monatliche Gebühren sowie ein paar Prozente Abschlag bei Zahlung 
an. Bei Interesse bitte mit mir Kontakt aufnehmen.

#********************************************************************************#


new feature in Oct 2023:

Payment with Nayax Onyx possible now. Please consider that because of the long boot time
of Onyx powersave must be disabled. 
How to wire: +12V, GND, Pulse 1, don't forget to pullup this to +5v;
To use Onyx you need beside the hardware (Onyx and Pulse-cable) an contract with Nayax for
credit card / payment clearing; this leads to an monthly fee and a percentage disagio for each
payment.
#********************************************************************************#

new feature in April 2024:
It is now possible to send & receive SMS-messages. 
You can receive messages about sold items or empty system. You can send messages to
change the threshold for this. You can send messages to load money onto the system and
finally it can be used for paypal-payments (QR-code or your paypal-account should be
visible for the client).
How does it work?
Deactivate the PIN of your simcard. I wasn't able to work with SMS using an IOT-Sim 
because my mobile phone provider doesn't deliver SMS from SIMs with international
phone number (not start e.g. with 49 for germany).
The automat sends a sms when automat is empty (threshold=0), 3 or less remaining 
(threshold=3) or for any sold item (threshold=99). You can send an SMS to your
system e.g. threshold=0 when you want to change the behaviour until the system is restarted.
Then the parameter which is set in the sketch is active again. 

!! don't use the powersupport from ardunino for the sim800L, it consumes too much power!!
Use a separate LM2596 and set this to 3,6V to supply SIM800L (never more than 4,2V)!!

It is now possible to pay with paypal. You have to avtivate "info via mail (html)" on paypal.com
website to receive an email about any received payment. With your email provider
you have to setup SMS-info towards your sim. Then the arduino will indentify how much
money was send and loads it onto the system. The value is visible on the display. It is 
possible to mix cash with paypal if the client wants it.
Preconfigered is any value to be loaded onto the system. Inform you clients
that Paypal-Payments need up to 30 seconds. You can create a QR-Code with a specific
amount of money using your paypal-app. During testing don't cancel your test sendings, because
Paypal blocked my first account and was not willing to reactivate. 
All activities are on your own risk!
Additional benefit: you can load money as well onto the system with sending an SMS containing 
the phrase "Sie haben x,00 EUR erhalten" in case of client is calling you because of 
whatever reason and you may want to give a free product or discount.
Because of this feature you should keep the phone number of your system confidentual.
#*********************************************************************************
Update 11.04.2024:
Nayax Onyx can now be inform about cash payments. I decided not to send the amount of
money by sending a number of pulses back but to send only one pulse (shown in 
nayax app as 1 EUR because number of empty compartments is more important than the
revenue. In configuration (nayax app or my.nayax.com in area "Pulse" set "Pulse IN1 Counter to 1" 
to allow your Nayax Onyx device to receive infos about cash payments. Please be patient, 
it needs some time until the cash payment is visible in you nayax app.
Special case: if a client combines Onyx and cash the last payment done will be used
to decide of a sold item will be reported as cash or not.
 
In addition you can now send "refill" via sms to the machine to activate the refill
routine wihtout opening the machine: all compartments which are registered as empty
will open and counters are set back to value written in the code.

If you have some of the additional hardware components e.g. NV10, Onyx, Sim800L please
use power supply with 2A. 

Update 16.05.2024:
to get Paypal payments we need to receive a SMS containing some key information.
As far as I found out a private Paypal account can be configured at the paypal
website to send sms after receiving money, for a business account you need to set 
up sms info with your mail provider, e.g. gmx is supporting this (9ct costs per sms). 
In the youtube video I show both ways how to setup the paypal. 
The QR code (containing a specific sum) can be generated using the paypal app.

###################################################################################


any questions? pls contact me: mailto:honigautomat@gmx.de
Bei Fragen helfe ich gern weiter: honigautomat@gmx.de





