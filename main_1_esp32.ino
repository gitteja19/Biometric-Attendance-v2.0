#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include "SPI.h"
#include "SD.h"
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "BluetoothSerial.h"


#define MAX_STRING_LENGTH 12
#define MAX_CAP 302
#define MAX_CLASS 40
#define MAX_COURSES 20

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

bool welcomePrinted = false;
///FINGERPRINT SENSOR///
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
//OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t id = 1;
int courseNo = 1;
char rollNumber[MAX_CAP][MAX_STRING_LENGTH] = { 0 };
char courseName[MAX_COURSES][MAX_STRING_LENGTH] = { 0 };
int courseClassNum[MAX_COURSES] = { 0 };


const char* ssid = "200020016";
const char* password = "12345678";




WiFiServer server(80);
WiFiClient client;

//BLUETOOTH PART
#define USE_PIN
const char* pin = "1234";
String device_name = "ESP32-BT-Slave";
BluetoothSerial SerialBT;


void setup() {

  Serial.begin(9600);
  Serial2.begin(115200);
  SerialBT.begin(device_name);

  while (!Serial)
    ;
  delay(100);

#ifdef USE_PIN
  SerialBT.setPin(pin);
#endif

  deviceSetup();
}

void loop() {
  delay(1000);
  Serial.println("looping");
  if (SerialBT.connected()) {
    displayText(0, 3, 2, "HEY THERE!", 0);
    displayText(0, 21, 1, "Bluetooth : ON", 1);
    if (!welcomePrinted) {
      SerialBT.println("Welcome! You are connected to the ESP32.");
      welcomePrinted = true;
    }
  } else {
    displayText(0, 3, 2, "HEY THERE!", 0);
    displayText(0, 21, 1, "Bluetooth : OFF", 1);
    if (welcomePrinted) {
      welcomePrinted = false;
    }
  }

  int kid = readFingerID();
  if (kid == 1) {
    SerialBT.println();
    SerialBT.println("*** HOST MODE ***");
    displayText(8, 8, 2, "HOST MODE", 0);
    SerialBT.println(F("\nEnter :\n  E for enroll\n  M to mark attendance\n  L to generate Attendance Log\n  D to download log\n  R to Factory reset\n  C to Add course\n"));
    char keyPress = readBluetoothData()[0];
    Serial.println(keyPress);
    if (keyPress == 'E' || keyPress == 'e') {
      // Code to handle enrollment

      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "ENROLLING!", 1);

      SerialBT.println(F("Enrollment selected."));
      SerialBT.println(F("Enter number of Enrollments"));
      int num = readBluetoothData().toInt();
      Serial.println(num);
      int curr_id = id;
      while (id - curr_id < num) {
        enroll();
      }
    } else if (keyPress == 'M' || keyPress == 'm') {

      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "ATTENDANCE!", 1);
      // Code to mark attendance
      SerialBT.println(F("Marking attendance selected."));
      SerialBT.println();
      SerialBT.println("Enter the class code index.");

      if (courseClassNum[0] == 0) {
        SerialBT.println("Add a course first!");
        return;
      }

      for (int ii = 1; ii <= MAX_COURSES; ii++) {
        if (courseClassNum[ii - 1] > 0) {
          SerialBT.println(String(ii) + " - " + String(courseName[ii - 1]));
        }
      }
      SerialBT.println();
      int in;
      while (true) {
        in = readBluetoothData().toInt();
        if (in > 0 && in < courseNo) {
          break;
        } else {
          SerialBT.println("Enter from above indexes.");
        }
      }
      SerialBT.println("Course selected - " + String(courseName[in - 1]));

      SerialBT.println("Is it class number = " + String(courseClassNum[in - 1]) + " ?");
      SerialBT.println("type y for yes, n for no.");

      while (true) {
        if (SerialBT.available() > 0) {
          const char userInput = readBluetoothData().c_str()[0];  // Read the first character
          Serial.println(userInput);

          if (userInput == 'y' || userInput == 'Y') {
            break;
          } else if (userInput == 'n' || userInput == 'N') {
            // User wants to change the class number
            SerialBT.println("Please enter the new class number:");
            courseClassNum[in - 1] = readBluetoothData().toInt();
            break;
          } else {
            // Invalid input, ask again
            SerialBT.println("Invalid input. Type 'y' to confirm, 'n' to change the class number:");
          }
        }
      }


      SerialBT.println(" Now taking attendence for class : " + String(courseClassNum[in - 1]));
      String filename = "/" + String(courseName[in - 1]) + "_" + String(courseClassNum[in - 1]) + ".txt";
      File stat = SD.open(filename, FILE_WRITE);
      if (stat) {
        stat.close();
        Serial.println("file creatde:" + filename);
      }

      unsigned long startTime = millis();      // Get the current time
      unsigned long duration = 1 * 60 * 1000;  // 1 minutes in milliseconds
      SerialBT.print(F("Place your finger to mark attendence!!..."));




      while (millis() - startTime < duration) {
        displayText(0, 8, 1, "Place finger\nto mark attendance..", 0);
        markAttendence(filename);
      }
      courseClassNum[in - 1]++;
      uploadClassNumsTxt("/classNums.txt");
      SerialBT.println("Done taking attendance.");
    } else if (keyPress == 'R' || keyPress == 'r') {
      // Code to reset
      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "RESET!", 1);
      Serial.print(F("Reset selected."));
      resetDevice();
    } else if (keyPress == 'L' || keyPress == 'l') {
      // Code to download
      Serial.print(F("creating log."));
      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "LOG MAKE!!", 1);
      SerialBT.println();
      if (courseClassNum[0] == 0) {
        SerialBT.println("Add a course first!");
        return;
      }
      SerialBT.println("enter the class code");

      for (int ii = 1; ii <= MAX_COURSES; ii++) {
        if (courseClassNum[ii - 1] > 0) {
          SerialBT.println(String(ii) + " - " + String(courseName[ii - 1]));
        }
      }
      SerialBT.println();
      int in;
      while (true) {
        in = readBluetoothData().toInt();
        if (in > 0 && in < courseNo) {
          break;
        } else {
          SerialBT.println("Enter from above indexes.");
        }
      }
      if(courseClassNum[in - 1] = 1)
      {
        SerialBT.println("Take atleast one class attendance first!");
        return;
      }
      makeAttendenceLog(in);
      SerialBT.println("Log done.");

    } else if (keyPress == 'D' || keyPress == 'd') {

      SerialBT.println(F("Setting up WiFi..."));
      SerialBT.println("Enter wifi SSID");
      String userName = readBluetoothData();
      Serial.println(userName);
      SerialBT.println("Enter wifi password");
      String passCode = readBluetoothData();



      WiFi.begin(userName, passCode);
      //WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
      }
      SerialBT.println("Wi-Fi connected!!");

      SerialBT.println();
      SerialBT.println("Enter the class code index.");

      for (int ii = 1; ii <= MAX_COURSES; ii++) {
        if (courseClassNum[ii - 1] > 0) {
          SerialBT.println(String(ii) + " - " + String(courseName[ii - 1]));
        }
      }
      SerialBT.println();
      int in;
      while (true) {
        in = readBluetoothData().toInt();
        if (in > 0 && in < courseNo) {
          break;
        } else {
          SerialBT.println("Enter from above indexes.");
        }
      }


      String path = "/" + String(courseName[in - 1]) + "_log.txt";
      SerialBT.println("Download selected.\n visit :" + WiFi.localIP().toString() + path);

      server.begin();

      SerialBT.println("place host finger after download");
      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "DOWNLOAD!", 1);
      while (readFingerID() != 1) {
        startFileDownload(path);
      }
      SerialBT.println("Download Done.");

    } else if (keyPress == 'C' || keyPress == 'c') {
      // mode add courseName
      displayText(0, 0, 2, "MODE :", 0);
      displayText(-1, 0, 2, "ADD COURSE!", 1);
      // enter courseName name
      SerialBT.println("Enter new course code:");

      String incomingData = readBluetoothData();
      strcpy(courseName[courseNo - 1], incomingData.c_str());
      courseClassNum[courseNo - 1] = 1;
      displayText(0, 5, 1, "COURSE : " + String(courseName[courseNo - 1]), 0);
      displayText(0, 18, 1, "Added!!", 1);
      SerialBT.println("Done.");


      // AtattendenceLogt to open "roll.txt" for writing
      File myFile_1 = SD.open("/courseName.txt", FILE_APPEND);
      if (myFile_1) {
        // Update roll numbers data
        char buffer[30];
        sprintf(buffer, "%s,%d", courseName[courseNo - 1], courseNo);
        myFile_1.println(buffer);
        myFile_1.close();
        courseNo++;
      } else {
        Serial.print(F("Error opening courseName.txt for writing"));
      }
      // update course num
      myFile_1 = SD.open("/courseNum.txt", FILE_WRITE);
      if (myFile_1) {
        myFile_1.println(courseNo);
        myFile_1.close();
      } else {
        Serial.print(F("Invalid selection."));
      }
      uploadClassNumsTxt("/classNums.txt");
    } else {
      Serial.println("try again.");
    }
  }
}

void deviceSetup() {
  //biometric start
  fingerSensorSetup();
  //display start
  displaySetup();
  //sd card start
  sdSetup();
  // update rolln_fingIdum array
  rollNumberArrayUpdate();
  // update id
  idUpdate();
  // course num update
  courseNumUpdate();
  // course Name array update
  courseNameArrayUpdate();
  //class nums
  classNumsArrayUpdate("/classNums.txt");
  //take host biometric
  if (id == 1) {
    enroll();
  }
}

void courseNameArrayUpdate() {
  File myFile_1 = SD.open("/courseName.txt", FILE_READ);

  // If the file is open, read data and populate rollN_fingIdumber array
  if (myFile_1) {
    while (myFile_1.available()) {
      // Read data from the file
      char buffer[30];
      myFile_1.readStringUntil('\n').toCharArray(buffer, sizeof(buffer));

      // Extract values using sscanf and populate rollN_fingIdumber[id]
      uint16_t ids;
      char roll[MAX_STRING_LENGTH];
      if (sscanf(buffer, "%[^,],%hu", roll, &ids) == 2 && ids <= MAX_COURSES) {
        strcpy(courseName[ids - 1], roll);
      } else {
        Serial.print(F("Error parsing data or invalid id"));
      }
    }

    myFile_1.close();  // Close the file
    Serial.println(F("\"courseName\" array updated!!"));
  } else {
    Serial.print(F("Error roll numbers update."));
  }
}

void courseNumUpdate() {
  File myFile_2 = SD.open("/courseNum.txt", FILE_READ);
  if (myFile_2) {
    courseNo = myFile_2.parseInt();
    myFile_2.close();  // Close the file
    Serial.print(F("\"courseNum\" count updated to :"));
    Serial.println(courseNo);
  } else {
    Serial.print(F("Error course update."));
  }
}

void fingerSensorSetup() {
  // set the data rate for the sensor serial port
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("finsor connected");
  } else {
    Serial.print(F("Did not find fingerprint sensor :("));
    while (1) { delay(1); }
  }
}

void displaySetup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);
  return;
}

void sdSetup() {
  Serial.print(F("Initializing SD card..."));

  if (!SD.begin(5, SPI, 4000000)) {
    return;
  }
  Serial.print(F("initialization SD done."));
}

//ROLL NUMBER ARRAY UPDATION FROM SD CARD
void rollNumberArrayUpdate() {
  // Open the file "roll.txt" in read mode
  File myFile_1 = SD.open("/roll.txt", FILE_READ);

  // If the file is open, read data and populate rollN_fingIdumber array
  if (myFile_1) {
    while (myFile_1.available()) {
      // Read data from the file
      char buffer[30];
      myFile_1.readStringUntil('\n').toCharArray(buffer, sizeof(buffer));

      // Extract values using sscanf and populate rollN_fingIdumber[id]
      uint16_t ids;
      char roll[MAX_STRING_LENGTH];
      if (sscanf(buffer, "%[^,],%hu", roll, &ids) == 2 && ids <= MAX_CAP) {
        strcpy(rollNumber[ids], roll);
      } else {
        Serial.print(F("Error parsing data or invalid id"));
      }
    }

    myFile_1.close();  // Close the file
    Serial.println(F("\"rollNumber\" array updated!!"));
  } else {
    Serial.print(F("Error roll numbers update."));
  }
}
//ID VARIABLE UPDATION FROM SD CARD
void idUpdate() {
  File myFile_2 = SD.open("/id.txt", FILE_READ);
  if (myFile_2) {
    id = myFile_2.parseInt();
    id++;
    myFile_2.close();  // Close the file
    Serial.print(F("\"id\" count updated to :"));
    Serial.println(id);
  } else {
    Serial.print(F("Error ID update."));
  }
}

//ENROLLMENT FUNTIONS
void enroll() {
  SerialBT.print(F("Ready to enroll a fingerprint!"));
  if (id == 1) {
    SerialBT.print(F("Host Fingerprint Password"));
    while (!getFingerprintEnroll())
      ;
    return;
  }
  while (!getFingerprintEnroll())
    ;
  return;
}

uint16_t getFingerprintEnroll() {
retry_enroll:
  int p = -1;
  Serial.print(F("Waiting for valid finger to enroll as #"));
  displayText(0, 13, 1, "place finger..", 0);
  SerialBT.println(F("place your finger !!"));
  Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image taken"));
        displayText(0, 13, 1, "Image taken.", 0);
        break;
      case FINGERPRINT_NOFINGER:
        SerialBT.print(F("."));
        break;
      default:
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      goto retry_enroll;
      return p;
    default:
      return p;
  }

  SerialBT.println(F("Remove finger"));
  displayText(0, 13, 1, "Remove finger.", 0);
  delay(1000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print(F("ID "));
  Serial.print(id);
  p = -1;
  SerialBT.println("");
  SerialBT.println(F("Place same finger again"));
  displayText(0, 9, 1, "Place same \nfinger again", 0);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image taken"));
        displayText(0, 13, 1, "Image taken.", 0);
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(F("."));
        break;
      default:
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      goto retry_enroll;
      return p;
    default:
      return p;
  }

  // OK converted!
  Serial.print(F("Creating model for #"));
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println(F("Prints matched!"));
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    goto retry_enroll;
    return p;
  } else {
    return p;
  }

  Serial.print(F("ID "));
  Serial.print(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    SerialBT.println(F("Stored!"));
    displayText(0, 13, 1, "Stored.", 0);
  } else {
    return p;
  }

  if (id != 1) {
    SerialBT.println("Enter the Roll no of the student:");
    displayText(0, 0, 1, "Enter Roll No.", 0);
    while (SerialBT.available() <= 0) {};
    String incomingData = SerialBT.readStringUntil('\n');
    incomingData.trim();
    if (incomingData.length() > 0) {
      Serial.print(incomingData);
      strcpy(rollNumber[id], incomingData.c_str());
      Serial.print(rollNumber[id]);
    }

    // AtattendenceLogt to open "roll.txt" for writing
    File myFile_1 = SD.open("/roll.txt", FILE_APPEND);
    if (myFile_1) {
      // Update roll numbers data
      char buffer[30];
      sprintf(buffer, "%s,%hu", rollNumber[id], id);
      myFile_1.println(buffer);
      myFile_1.close();
    } else {
      Serial.print(F("Error opening roll.txt for writing"));
    }

    myFile_1 = SD.open("/log_format.txt", FILE_APPEND);
    if (myFile_1) {
      myFile_1.println(rollNumber[id]);
      myFile_1.close();
    } else {
      Serial.print(F("Error opening roll.txt for writing"));
    }

    // update id to "id.txt"
    File myFile_2 = SD.open("/id.txt", FILE_WRITE);
    if (myFile_2) {
      myFile_2.println(id);
      myFile_2.close();
    } else {
      Serial.print(F("Error opening id.txt"));
    }

    SerialBT.print(F("Biometric has been registered of the student: "));
    SerialBT.println(rollNumber[id]);
    displayText(0, 4, 1, "ID : " + String(rollNumber[id]), 0);
    displayText(0, 19, 1, "Enrolled.", 1);
    delay(2000);
  } else {
    Serial.println(F("Host Fingerprint has been Registered"));
    displayText(40, 5, 2, "HOST", 0);
    displayText(0, 20, 1, "-Assigned.", 1);
    delay(2000);
    File myFile_1 = SD.open("/roll.txt", FILE_WRITE);
    myFile_1.close();
    myFile_1 = SD.open("/courseName.txt", FILE_WRITE);
    myFile_1.close();
    myFile_1 = SD.open("/log_format.txt", FILE_WRITE);
    myFile_1.println("ROLL NUMBER");
    myFile_1.close();
    File myFile_2 = SD.open("/id.txt", FILE_WRITE);
    if (myFile_2) {
      myFile_2.println(id);
      myFile_2.close();
    } else {
      Serial.print(F("Error opening id.txt"));
    }

  }

  id += 1;
  return true;
}

//ATTENDENCE MARKING FUNCTIONS
int readFingerID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;
  return static_cast<int>(finger.fingerID);  // Also cast here
}
void markAttendence(String filename) {
  int key = readFingerID();
  if (key != -1) {
    File file = SD.open(filename, FILE_APPEND);
    if (file) {
      //file.printf("%d,%d\n", class_no, key);
      file.printf("%d\n", key);
      file.close();
      displayText(0, 5, 1, "ID : " + String(rollNumber[key]), 0);
      displayText(0, 18, 1, "Marked!!", 1);
      delay(1500);
    } else {
      Serial.print("Unable to open" + filename);
    }
  }
}

//BLUETOOTH SERIAL FUNCTION
String readBluetoothData() {
  String incomingData = "";
  while (SerialBT.available() <= 0) {
    // Wait for data to be available
  }

  // Read data until a newline character is encountered
  while (SerialBT.available() > 0) {
    char c = SerialBT.read();
    if (c == '\n') {
      // Newline character found, stop reading
      break;
    }
    incomingData += c;
    delay(5);
  }

  // Remove leading and trailing whitespace
  incomingData.trim();

  return incomingData;
}



int findIDByRollNumber(const char* targetRollNumber) {
  for (int i = 1; i <= id; i++) {
    if (strcmp(rollNumber[i], targetRollNumber) == 0) {
      return i;  // Found a match, return the ID
    }
  }
  // If no match is found, return -1 (indicating not found)
  return -1;
}

// ATTENDANCE LOG CREATER
void makeAttendenceLog(int cid) {

  if(courseClassNum[cid-1]==1)
  {
    SerialBT.println("Take attendance for atleast one class before making log.");
    return;
  }
  for (int ii = 1; ii < courseClassNum[cid - 1]; ii++) {
    int keyStat[id - 2] = { -1 };
    String filename = "/" + String(courseName[cid - 1]) + "_" + String(ii) + ".txt";
    File myFile = SD.open(filename, FILE_READ);
    if (!myFile) {
      Serial.println("Can't read" + filename);
      return;
    }
    while (myFile.available()) {
      int readId = myFile.parseInt();
      if (readId != 1) {
        keyStat[readId - 2] = 1;
      }
    }
    myFile.close();

    myFile = SD.open("/log_format.txt", FILE_READ);
    if (!myFile) {
      Serial.println("Can't read log_format");
      return;
    }

    String heading = myFile.readStringUntil('\n');
    heading.trim();
    String sheetName = "/" + String(courseName[cid - 1]) + "_log.txt";
    if (ii == 1) {
      File log = SD.open(sheetName, FILE_WRITE);
      heading = heading + ",class " + String(ii);
      log.println(heading);
      while (myFile.available()) {
        heading = myFile.readStringUntil('\n');
        heading.trim();
        int rr = findIDByRollNumber(heading.c_str());
        if (rr != 1) {
          if (keyStat[rr - 2] > 0) {
            heading = heading + ",present";
          } else {
            heading = heading + ",absent";
          }
          log.println(heading);
        }
      }
      myFile.close();
      log.close();
    } else {
      File temp = SD.open("/temp.txt", FILE_WRITE);
      File log = SD.open(sheetName, FILE_READ);
      if (!log) {
        Serial.println("Can't read log");
        return;
      }
      heading = log.readStringUntil('\n');
      heading.trim();
      heading = heading + ",class " + String(ii);
      temp.println(heading);
      while (myFile.available()) {
        heading = myFile.readStringUntil('\n');
        heading.trim();
        int rr = findIDByRollNumber(heading.c_str());
        heading = log.readStringUntil('\n');
        heading.trim();
        if (rr != 1) {
          if (keyStat[rr - 2] > 0) {
            heading = heading + ",present";
          } else {
            heading = heading + ",absent";
          }
          temp.println(heading);
        }
      }
      myFile.close();
      log.close();
      temp.close();
      SD.remove(sheetName);
      SD.rename("/temp.txt", sheetName);
    }
  }
}

// void makeAttendenceLog() {

//   for (int ii = 1; ii < classNo; ii++) {
//     int keyStat[id - 2] = { -1 };
//     String filename = "/class_" + String(ii) + ".txt";
//     File myFile = SD.open(filename, FILE_READ);
//     if (!myFile) {
//       Serial.println("Can't read" + filename);
//       return;
//     }
//     while (myFile.available()) {
//       int readId = myFile.parseInt();
//       keyStat[readId - 2] = 1;
//     }
//     myFile.close();

//     myFile = SD.open("/log_format.txt", FILE_READ);
//     if (!myFile) {
//       Serial.println("Can't read log_format");
//       return;
//     }

//     String heading = myFile.readStringUntil('\n');
//     heading.trim();
//     if (ii == 1) {
//       File log = SD.open("/sheet.txt", FILE_WRITE);
//       heading = heading + ",class " + String(ii);
//       log.println(heading);
//       while (myFile.available()) {
//         heading = myFile.readStringUntil('\n');
//         heading.trim();
//         int rr = findIDByRollNumber(heading.c_str());
//         if (rr != 1) {
//           if (keyStat[rr - 2] > 0) {
//             heading = heading + ",present";
//           } else {
//             heading = heading + ",absent";
//           }
//           log.println(heading);
//         }
//       }
//       myFile.close();
//       log.close();
//     } else {
//       File temp = SD.open("/temp.txt", FILE_WRITE);
//       File log = SD.open("/sheet.txt", FILE_READ);
//       if (!log) {
//         Serial.println("Can't read log");
//         return;
//       }
//       heading = log.readStringUntil('\n');
//       heading.trim();
//       heading = heading + ",class " + String(ii);
//       temp.println(heading);
//       while (myFile.available()) {
//         heading = myFile.readStringUntil('\n');
//         heading.trim();
//         int rr = findIDByRollNumber(heading.c_str());
//         heading = log.readStringUntil('\n');
//         heading.trim();
//         if (rr != 1) {
//           if (keyStat[rr - 2] > 0) {
//             heading = heading + ",present";
//           } else {
//             heading = heading + ",absent";
//           }
//           temp.println(heading);
//         }
//       }
//       myFile.close();
//       log.close();
//       temp.close();
//       SD.remove("/sheet.txt");
//       SD.rename("/temp.txt", "/sheet.txt");
//     }
//   }
// }

void startFileDownload(String filePath) {
  client = server.accept();

  if (client) {
    Serial.print(F("New client connected"));

    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        currentLine += c;
        if (c == '\n') {
          if (currentLine.startsWith("GET")) {
            if (currentLine.indexOf(filePath) != -1) {
              Serial.print(F("File download requested"));

              // Requested path matches the file
              File file = SD.open(filePath);
              if (file) {
                Serial.print(F("File opened successfully"));

                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/plain");
                client.print("Content-Disposition: attachment; filename=\"" + String(filePath) + "\"\r\n");
                client.println("Content-Length: " + String(file.size()));
                client.println();

                while (file.available()) {
                  client.write(file.read());
                }

                file.close();
                Serial.print(F("File sent successfully"));
              } else {
                Serial.print(F("Error opening file"));
                client.println("HTTP/1.1 404 Not Found");
              }
            } else {
              client.println("HTTP/1.1 404 Not Found");
            }
          }
          break;
        }
      }
    }

    client.flush();
    client.stop();
    Serial.print(F("Client disconnected"));
  }
}

// void startFileDownload(const char* filePath) {
//   client = server.accept();

//   if (client) {
//     Serial.print(F("New client connected"));

//     String currentLine = "";
//     while (client.connected()) {
//       if (client.available()) {
//         char c = client.read();
//         currentLine += c;
//         if (c == '\n') {
//           if (currentLine.startsWith("GET")) {
//             if (currentLine.indexOf(filePath) != -1) {
//               Serial.print(F("File download requested"));

//               // Requested path matches the file
//               File file = SD.open(filePath);
//               if (file) {
//                 Serial.print(F("File opened successfully"));

//                 client.println("HTTP/1.1 200 OK");
//                 client.println("Content-Type: text/plain");
//                 client.print("Content-Disposition: attachment; filename=\"" + String(filePath) + "\"\r\n");
//                 client.println("Content-Length: " + String(file.size()));
//                 client.println();

//                 while (file.available()) {
//                   client.write(file.read());
//                 }

//                 file.close();
//                 Serial.print(F("File sent successfully"));
//               } else {
//                 Serial.print(F("Error opening file"));
//                 client.println("HTTP/1.1 404 Not Found");
//               }
//             } else {
//               client.println("HTTP/1.1 404 Not Found");
//             }
//           }
//           break;
//         }
//       }
//     }

//     client.flush();
//     client.stop();
//     Serial.print(F("Client disconnected"));
//   }
// }

void classNumsArrayUpdate(const char* fileName) {
  // Open the file
  File file = SD.open(fileName, FILE_READ);

  // Check if the file opened successfully
  if (file) {
    while (file.available()) {
      // Read a line from the file
      String line = file.readStringUntil('\n');

      // Split the line into key and value
      int commaIndex = line.indexOf(',');
      if (commaIndex != -1) {
        String keyString = line.substring(0, commaIndex);
        String valueString = line.substring(commaIndex + 1);

        // Convert key and value to integers
        int key = keyString.toInt();
        int value = valueString.toInt();

        // Update the array
        if (key >= 1 && key <= MAX_COURSES) {
          courseClassNum[key - 1] = value;
        }
      }
    }

    // Close the file
    file.close();
  } else {
    // Print an error message if the file couldn't be opened
    Serial.println("Error opening file");
  }
}

void uploadClassNumsTxt(const char* fileName) {
  // Open the file for writing
  File file = SD.open(fileName, FILE_WRITE);

  // Check if the file opened successfully
  if (file) {
    // Write each key-value pair to the file
    for (int i = 0; i < MAX_COURSES; ++i) {
      // Write key-value pair in the format "key,value"
      file.print(i + 1);  // Key (course number)
      file.print(",");
      file.println(courseClassNum[i]);  // Value (class number)
    }

    // Close the file
    file.close();
    Serial.println("Array written to file");
  } else {
    // Print an error message if the file couldn't be opened
    Serial.println("Error opening file for writing");
  }
}

void resetDevice() {
  SerialBT.println("Place Host fingerprint.");

  // Wait for the host to place the finger
  while (readFingerID() != 1) {
    delay(500);
  }

  SerialBT.println("Fingerprint recognized. Do you want to reset the device? (Y/N)");

  // Wait for the host to confirm
  while (true) {
    if (SerialBT.available() > 0) {
      char response = SerialBT.read();
      if (response == 'Y' || response == 'y') {
        SerialBT.println("Resetting device...");
        displayText(0, 5, 2, "RESETING..", 0);
        displayText(0, 20, 1, "Please wait!!", 1);
        finger.emptyDatabase();
        resetSD();
        ESP.restart();
      } else if (response == 'N' || response == 'n') {
        SerialBT.println("Reset canceled.");
        displayText(0, 5, 2, "RESET", 0);
        displayText(0, 20, 1, "Aborted!!", 1);
        break;
      }
    }
    delay(500);
  }
}

void resetSD() {
  File dir = SD.open("/");

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    Serial.print("Removing ");
    Serial.println(entry.name());
    String name = "/" + String(entry.name());
    SD.remove(name);
    entry.close();
  }

  dir.close();
  Serial.println("All files deleted");
}

void displayText(int x, int y, int size, String text, int stat) {
  if (stat == 0) {
    display.clearDisplay();
  }
  display.setTextSize(size);
  display.setTextColor(WHITE);
  if (x >= 0) {
    display.setCursor(x, y);
  }
  display.println(text);
  display.display();
}

/*4
//DONE
1.ENROLL
2.TAKE ATTENDENCE AND CREATE SEPERATE TXT FILE FOR EACH CLASS and confirms class num
3.ATTENDENCE LOG SHEET CREATION
4.LOG SHEET DOWNLOAD
//TO DO
3.DISPLAY
4.BUZZER
//MAY DO
2.HAPTIC FEEDBACK
*/


/*
1.wrote makeAttendenceLog(), didn't check it.
2.checked it , working!!.
3. hurray memory isuue resolved.
4. nxt have to do, ask wifi cred when dowloading, proper download UI,
*/
