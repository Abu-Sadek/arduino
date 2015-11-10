/* Arduino nano/uno connections:
 *  pin D9 -> D2 : clock tick timer
 *  pin A2 -> connects to output of buttons circuit
 *  pin D7 -> connects to L9110 Motor-1 in-signal connection, the other Motor-1 in-signal is connected to GND
 *  pin D6 -> LCD 11
 *  pin D5 -> LCD 12
 *  pin D4 -> LCD 13
 *  pin D3 -> LCD 14
 *  pin D11 -> LCD 6
 *  pin D12 -> LCD 4
 */


// include the library code:
#include <LiquidCrystal.h>

#define BUTTON_DOWN 1
#define BUTTON_UP   2
#define BUTTON_OK   3
#define BUTTON_CANCEL 4
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 6, 5, 4, 3);

byte hours = 0 ;
volatile byte seconds = 0 ;
volatile byte minutes = 0 ;
volatile int masterClock = 0 ;
int icounter = 0 ;
volatile unsigned long int tseconds = 0 ;

enum SystemStates { 
  State_NoButton = 0 ,
  State_Initialize , 
  State_ShowTime,
  State_MainMenu_Initialize,
  State_MainMenu_Run,
  State_SetHour_Initialize,
  State_SetHour_Run,
  State_SetMinute_Initialize,
  State_SetMinute_Run,
  State_AddAppointment_Initialize,
  State_AddAppointment_GetTitle_Run,
  State_AddAppointment_GetHour_Init,
  State_AddAppointment_GetHour_Run,
  State_AddAppointment_GetMinute_Init,
  State_AddAppointment_GetMinute_Run,
  State_AddAppointment_Done,
  State_Appointment_Alarm_Init,
  State_Appointment_Alarm_Run
} ;


void setup() {
  // set up the LCD's number of columns and rows:
  pinMode(9, OUTPUT) ;
  pinMode(7, OUTPUT) ;
  analogWrite(9, 127) ;
  lcd.begin(40, 2);
  Serial.begin(9600) ;
  
  pinMode(2, INPUT) ;
  attachInterrupt(digitalPinToInterrupt(2), clockCounter, CHANGE);

  pinMode(A2, INPUT) ;
}

void print2(byte v)
{
  if(v < 10)
    lcd.print("0") ;
  lcd.print(v) ;
}

void printTime(byte x, byte y)
{
  lcd.setCursor(x, y) ;
  print2(hours);
  lcd.print(":") ;
  print2(minutes);
  lcd.print(":") ;
  print2(seconds) ;
  lcd.print("  ") ;
}

void adjustTimeCounters()
{
  if(seconds == 60)
  {
    seconds = 0 ;
    minutes++ ;
  }
  if(minutes == 60)
  {
    minutes = 0 ;
    hours++ ;
  }
  if(hours == 24)
  {
    hours = 0 ;
  }
}

void clockCounter()      // called by interrupt
{
  masterClock ++;        // with each clock rise add 1 to masterclock count
  if(masterClock == 980) // 490Hz reached     
  {                         
    tseconds ++;          // after one 490Hz cycle add 1 second ;)
    seconds++ ;
    masterClock = 0;     // Reset after 1 second is reached
   }
  return;
}

byte getPressedButton()
{
  int c = analogRead(A2) ;
  Serial.println(c) ;
  byte rval = 0 ;
  if(c > 990) {
    Serial.println("button 1 pressed") ;
    rval = 1 ;
  }
  else if(c > 900) {
    Serial.println("button 2 pressed") ;
    rval = 2 ;
  }
  else if(c > 600) {
    Serial.println("button 3 pressed") ;
    rval = 3 ;
  }
  else if(c > 500) {
    Serial.println("button 4 pressed") ;
    rval = 4 ; 
  }
  delay(200) ;
  return rval ;
}

char *mainMenu[] = { "Set Hour", "Set Minute", "Set Appointment" } ;
char mainMenuItemCount = 3 ;
byte mainMenuFocus = 0 ;

byte newHours = 0 ;
byte newMinutes = 0 ;

byte currentState = State_Initialize ;
byte nextState = State_Initialize ;

char appoints_title[10][31] ;
byte appoints_hour[10] ;
byte appoints_minute[10] ;
byte appoints_dismissed[10] ;
byte appoints_size = sizeof(appoints_hour)/sizeof(appoints_hour[0]);
byte nextApp = 0 ;
byte appoints_alarm = 0;

char testbuff[30] ;

void printMainMenu402()
{
  lcd.clear() ;
  lcd.setCursor(2,0) ;
  lcd.print("Main Menu") ;
  lcd.setCursor(0,1) ;
  if(mainMenuFocus > 0)
    lcd.print("< ") ;
  else
    lcd.print("  ") ;
  lcd.print(mainMenu[mainMenuFocus]) ;
}

byte getMainMenuChoice()
{
    byte button = getPressedButton() ;

  if(button == BUTTON_DOWN)
  {
    if(mainMenuFocus+1 < mainMenuItemCount)
      mainMenuFocus++ ;
    return 254 ;
  }
  if(button == BUTTON_UP)
  {
    if(mainMenuFocus > 0)
      mainMenuFocus-- ;
    return 254 ;
  }
  if(button == BUTTON_OK)
  {
    return mainMenuFocus+1 ;
  }
  if(button == BUTTON_CANCEL)
  {
    return 255 ;
  }
  return 0 ;
}

char *getString_buff ;
byte getString_limit ;
byte getString_index ;
byte getString_barIndex ;
char *getString_bar = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" ;
char *getString_barChar = " " ;
byte getString_barSize = 36 ;
byte getString_x = 0 ;
byte getString_y = 0 ;
byte prevButton = 0 ;

void getString_Initialize(byte x, byte y, char* target, byte limit)
{
  for(int i=0;i<limit;++i)
    target[i] = ' ' ;
  target[limit] = 0 ;
  getString_limit = limit ;
  getString_buff = target ;
  getString_index = 0 ;
  getString_barIndex = 0;
  getString_x = x ;
  getString_y = y ;
  prevButton = 0 ;
}

byte getString_Run()
{
  bool isChange = false ;
  byte button = getPressedButton() ;
  if(button == prevButton)
  {
    if(button == BUTTON_CANCEL)
      return BUTTON_CANCEL ;
    if(button == BUTTON_OK)
      return BUTTON_OK ;
  }
  prevButton = button ;

  if(button == BUTTON_OK)
  {
    if(getString_index < getString_limit)
    {
      getString_buff[getString_index] = getString_bar[getString_barIndex] ;
      getString_index++ ;
      getString_barIndex = 0 ;
      isChange = true ;
    }
  }

  if(button == BUTTON_CANCEL)
  {
    getString_buff[getString_index] = ' ';
    if(getString_index > 0)
      getString_index-- ;
    getString_buff[getString_index] = ' ';
    getString_barIndex = 0 ;
    isChange = true ;
  }

  if(button == BUTTON_DOWN)
  {
    getString_barIndex++ ;
    if(getString_barIndex == getString_barSize)
      getString_barIndex = 0 ;
    isChange = true ;  
  }

  if(button == BUTTON_UP)
  {
    if(getString_barIndex > 0)
      getString_barIndex-- ;
    else
      getString_barIndex = getString_barSize-1 ;
    isChange = true ;
  }

  if(isChange)
  {
    lcd.setCursor(getString_x, getString_y) ;
    lcd.print(getString_buff) ;
    getString_barChar[0] = getString_bar[getString_barIndex] ;
    lcd.setCursor(getString_x + getString_index, getString_y) ;
    lcd.print(getString_barChar) ;
    lcd.setCursor(getString_x + getString_index, getString_y) ;
    lcd.cursor() ;
  }

  return 0 ;
}

byte get2Digits_x ;
byte get2Digits_y ;
byte get2Digits_value ;
byte get2Digits_limit ;

void get2Digits_Init(byte x, byte y, byte initValue, byte limit)
{
  lcd.setCursor(x,y) ;
  get2Digits_x = x ;
  get2Digits_y = y ;
  get2Digits_value = initValue ;
  get2Digits_limit = limit ;
}

byte get2Digits_Run()
{
  lcd.setCursor(get2Digits_x, get2Digits_y) ;
  print2(get2Digits_value) ;
  byte selection = getPressedButton() ;
  if(selection == BUTTON_DOWN)
  {
    if(get2Digits_value == get2Digits_limit-1)
      get2Digits_value = 0 ;
    else
      get2Digits_value++ ;
  } else if(selection == BUTTON_UP) 
  {
    if(get2Digits_value > 0)
      get2Digits_value-- ;
    else
      get2Digits_value = get2Digits_limit-1; 
  } else if(selection == BUTTON_OK)
  {
    return BUTTON_OK ;
  }
  else if(selection == BUTTON_CANCEL)
  {
    return BUTTON_CANCEL ;
  }
  return 0 ;
}

void setNextState(byte state)
{
  currentState = State_NoButton ;
  nextState = state ;
}

byte getNonDismissedAppoints()
{
  byte r = 0 ;
  for(int i=0;i<nextApp;++i)
    if(appoints_dismissed[i] == 0)
      ++r ;
  return r ;
}

void loop() {
  adjustTimeCounters() ;

  byte selection = 0 ;

  switch(currentState)
  {
    case State_NoButton:
      if(getPressedButton() == 0)
        currentState = nextState ;
      break ;
      
    case State_Initialize:
      lcd.clear() ;
      currentState = State_ShowTime ;
      lcd.print("  Appointement Manager") ;
      lcd.setCursor(20,1) ;
      lcd.print(" appointments: ");
      lcd.print(getNonDismissedAppoints()) ;
    
    case State_ShowTime:
      printTime(2,1) ;
      selection = getPressedButton() ;
      if(selection == BUTTON_OK)
        currentState = State_MainMenu_Initialize ;       
      for(int i=0;i<nextApp;++i)
        if(appoints_hour[i] == hours && appoints_minute[i] == minutes && appoints_dismissed[i] == 0) {
          currentState = State_Appointment_Alarm_Init ;
          appoints_alarm = i+1 ;
          break ;
        }
      break ;
    
    case State_MainMenu_Initialize:
      printMainMenu402() ;
      setNextState(State_MainMenu_Run) ;
      break ;
    
    case State_MainMenu_Run:
      selection = getMainMenuChoice() ;
      if(selection == 255)
        currentState = State_Initialize ;
      else if(selection == 254)
        currentState = State_MainMenu_Initialize ;
      else if(selection == 1)
        currentState = State_SetHour_Initialize ;
      else if(selection == 2)
        currentState = State_SetMinute_Initialize ;
      else if(selection == 3)
        currentState = State_AddAppointment_Initialize ;
      break ;
    
    case State_SetHour_Initialize:
      lcd.clear() ;
      lcd.print("Set Hour") ;
      get2Digits_Init(2, 1, hours, 24) ;
//      lcd.setCursor(0,1) ;
//      lcd.print("  ") ;
//      newHours = hours ;
      setNextState(State_SetHour_Run);
      break ;
      
    case State_SetHour_Run:
      selection = get2Digits_Run() ;
      if(selection == BUTTON_OK)
      {
        hours = get2Digits_value ;
        setNextState(State_Initialize) ;
      }
      else if(selection == BUTTON_CANCEL)
      {
        currentState = State_Initialize ;
      }
      break ;
    
    case State_SetMinute_Initialize:
      lcd.clear() ;
      lcd.print("Set Minute") ;
      get2Digits_Init(2, 1, minutes, 60) ;
      setNextState(State_SetMinute_Run);
      break ;
      
    case State_SetMinute_Run:
      selection = get2Digits_Run() ;
      if(selection == BUTTON_OK)
      {
        minutes = get2Digits_value ;
        setNextState(State_Initialize) ;
      }
      else if(selection == BUTTON_CANCEL)
      {
        setNextState(State_Initialize) ;
      }
      break ;

    case State_AddAppointment_Initialize:
      if(nextApp >= appoints_size)
      {
        currentState = State_MainMenu_Initialize ;
        break ;
      }
      lcd.clear() ;
      lcd.print("  Add Appointment") ;
      lcd.setCursor(0,1) ;
      lcd.print("Title: ") ;
      getString_Initialize(8, 1, appoints_title[nextApp], appoints_size-1) ;
      lcd.cursor() ;
      setNextState(State_AddAppointment_GetTitle_Run) ;
      break ;
      
    case State_AddAppointment_GetTitle_Run:
      selection = getString_Run() ;
      if(selection == BUTTON_CANCEL)
      {
        currentState = State_Initialize ;
        lcd.noCursor() ;
        break ;
      } else if(selection == BUTTON_OK)
      {
        setNextState(State_AddAppointment_GetHour_Init);
        lcd.noCursor() ;
        break ;
      }
      break ;
      
    case State_AddAppointment_GetHour_Init:
      lcd.setCursor(20, 0) ;
      lcd.print(" - Alarm Time: ") ;
      get2Digits_Init(35, 0, hours, 24) ;
      lcd.setCursor(37,0);
      lcd.print(":") ;
      print2(minutes) ;
      appoints_hour[nextApp] = hours ;
      setNextState(State_AddAppointment_GetHour_Run) ;
      lcd.cursor() ;
      lcd.setCursor(35,0) ;
      break ;
      
    case State_AddAppointment_GetHour_Run:
      selection = get2Digits_Run() ;
      if(selection == BUTTON_CANCEL)
      {
        setNextState(State_AddAppointment_GetTitle_Run) ;
        lcd.cursor() ;
      }
      if(selection == BUTTON_OK)
      {
        setNextState(State_AddAppointment_GetMinute_Init) ;
        appoints_hour[nextApp] = get2Digits_value ;
      }
      lcd.setCursor(35,0) ;
      break ;

    case State_AddAppointment_GetMinute_Init:
      lcd.setCursor(38,0) ;
      get2Digits_Init(38, 0, minutes, 60) ;
      appoints_minute[nextApp] = minutes ; 
      setNextState(State_AddAppointment_GetMinute_Run) ;
      lcd.cursor() ;
      lcd.setCursor(38,0) ;
      break ;

    case State_AddAppointment_GetMinute_Run:
      selection = get2Digits_Run() ;
      if(selection == BUTTON_CANCEL)
      {
        setNextState(State_AddAppointment_GetHour_Init) ;
        lcd.cursor() ;
      }
      if(selection == BUTTON_OK)
      {
        currentState = State_AddAppointment_Done ;
        appoints_minute[nextApp] = get2Digits_value ;
        appoints_dismissed[nextApp] = 0 ;
        nextApp++ ;
      }
      lcd.setCursor(38,0) ;
      break ;

      case State_AddAppointment_Done:
        lcd.clear() ;
        lcd.noCursor() ;
        lcd.print("Appointment added at ") ;
        print2(appoints_hour[nextApp-1]) ;
        lcd.print(":") ;
        print2(appoints_minute[nextApp-1]) ;
        lcd.setCursor(0,1) ;
        lcd.print(appoints_title[nextApp-1]) ;
        delay(5000) ;
        setNextState(State_Initialize) ;
        break ;       
         
      case State_Appointment_Alarm_Init:
        lcd.clear() ;
        lcd.print("Appointment Alarm: ") ;
        lcd.print(appoints_title[appoints_alarm-1]) ;
        lcd.setCursor(0,1) ;
        lcd.print("Press OK to dismiss") ;
        appoints_dismissed[appoints_alarm-1] = 1 ;
        setNextState(State_Appointment_Alarm_Run) ;
        digitalWrite(7, HIGH);
        break ;

      case State_Appointment_Alarm_Run:
        selection = getPressedButton() ;
        if(selection == BUTTON_OK)
        {
          digitalWrite(7, LOW);
          setNextState(State_Initialize) ;
        }
        break ;
  }  
}


