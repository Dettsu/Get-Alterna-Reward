/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

#include "Joystick.h"

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	#ifdef ALERT_WHEN_DONE
	// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
	#warning LED and Buzzer functionality enabled. All pins on both PORTB and \
PORTD will toggle when printing is done.
	DDRD  = 0xFF; //Teensy uses PORTD
	PORTD =  0x0;
                  //We'll just flash all pins on both ports since the UNO R3
	DDRB  = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB =  0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
	#endif
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while(Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError);
			// At this point, we can react to this data.

			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

typedef enum {
	SYNC_POSITION,
	BREATHE,
	PROCESS,
	DONE
} State_t;
State_t state = SYNC_POSITION;

typedef enum {
	CONNECT_CONTROLLER,
	SYNC_CONTROLLER,
	GO_TO_ALTERNA,
	OPEN_OPTION,
	TURN_OFF_GYRO,
	SET_SENSITIVITY,
	JUMP_TO_STAGE,
	ENTER_STAGE,
	CLEAR_STAGE,
	LUNCH_DRONE,
	RESET_SENSITIVITY,
	RESET_GYRO_SETTING,
	BACK_TO_SPLATSVILLE,
} Step_t;
Step_t step = CONNECT_CONTROLLER;

command tmp;

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int bufindex = 0;

int duration_count = 0;
int report_count = 0;

int cnt = 0;
int flag = 0;
int mode = 0;
int clear_count = 0; // ステージ1-8をクリアした回数をカウント
int sensitivity_set = -5 * 2; // プログラムで用いる操作感度を2倍
int sensitivity_val = SENSITIVITY * 2;

int portsval = 0;

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData) {

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;

	// Repeat ECHOES times the last report
	if (echoes > 0)
	{
		memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
		echoes--;
		return;
	}

	// States and moves management
	switch (state)
	{
		
		case SYNC_POSITION:
			bufindex = 0;
			
			ReportData->Button = 0;
			ReportData->LX = STICK_CENTER;
			ReportData->LY = STICK_CENTER;
			ReportData->RX = STICK_CENTER;
			ReportData->RY = STICK_CENTER;
			ReportData->HAT = HAT_CENTER;
			
			state = BREATHE;
			break;
		
		case BREATHE:
			state = PROCESS;
			break;
		
		case PROCESS:
			
			switch (step) {

				case CONNECT_CONTROLLER:
					tmp = ConnectController(bufindex);
					break;
				
				case SYNC_CONTROLLER:
					tmp = SyncController(bufindex);
					break;
				
				case GO_TO_ALTERNA:
					tmp = GoToAlterna(bufindex);
					break;
				
				case OPEN_OPTION:
					tmp = OpenOption(bufindex);

					if (tmp.button == END) {
						cnt++;
					}

					break;
				
				case TURN_OFF_GYRO:
					if (GYRO_SETTING) {
						tmp = TurnOffGyro(bufindex);
					}
					
					break;
				
				case SET_SENSITIVITY:
					flag = 0;
					if (sensitivity_set != sensitivity_val) {
						if (SENSITIVITY * 2 > sensitivity_set) {
							mode = 1;
							tmp = SetSensitivity(bufindex, mode); // 十字左連打
						}
						if (SENSITIVITY * 2 < sensitivity_set) {
							mode = 0;
							tmp = SetSensitivity(bufindex, mode); // 十字右連打
						}
						if (tmp.button == END && mode == 1) {
							sensitivity_val--;
						}
						if (tmp.button == END && mode == 0) {
							sensitivity_val++;
						}
					} else {
						flag = 1;
					}

					break;
				
				case JUMP_TO_STAGE:
					tmp = JumpToStage(bufindex);
					break;

				case ENTER_STAGE:
					tmp = EnterStage(bufindex);
					break;
				
				case CLEAR_STAGE:
					tmp = ClearStage(bufindex);

					if (tmp.button == END) {
						clear_count++;
					}

					break;
				
				case LUNCH_DRONE:
					tmp = LunchDrone(bufindex);
					break;
				
				case RESET_SENSITIVITY:
					flag = 0;
					if (sensitivity_val != SENSITIVITY * 2) {
						if (SENSITIVITY * 2 > sensitivity_set) {
							mode = 0;
							tmp = SetSensitivity(bufindex, mode); // 十字右連打
						}
						if (SENSITIVITY * 2 < sensitivity_set) {
							mode = 1;
							tmp = SetSensitivity(bufindex, mode); // 十字左連打
						}
						if (tmp.button == END && mode == 0) {
							sensitivity_val++;
						}
						if (tmp.button == END && mode == 1) {
							sensitivity_val--;
						}
					} else {
						flag = 1;
					}

					break;
				
				case RESET_GYRO_SETTING:
					if (GYRO_SETTING) {
						tmp = ResetGyroSetting(bufindex);
					}
					
					break;
				
				case BACK_TO_SPLATSVILLE:
					tmp = BackToSplatsville(bufindex);

					if (tmp.button == END) {
						state = DONE;
					}

					break;
				
			}

			switch (tmp.button) {
				
				case L_UP:
					ReportData->LY = STICK_MIN;
					break;
				
				case L_DOWN:
					ReportData->LY = STICK_MAX;
					break;
				
				case L_LEFT:
					ReportData->LX = STICK_MIN;
					break;
				
				case L_RIGHT:
					ReportData->LX = STICK_MAX;
					break;
				
				case R_UP:
					if (!REVERSE_UD) {
						ReportData->RY = STICK_MIN;
					} else {
						ReportData->RY = STICK_MAX;
					}
					break;
				
				case R_DOWN:
					if (!REVERSE_UD) {
						ReportData->RY = STICK_MAX;
					} else {
						ReportData->RY = STICK_MIN;
					}
					break;
				
				case R_LEFT:
					if (!REVERSE_LR) {
						ReportData->RX = STICK_MIN;
					} else {
						ReportData->RX = STICK_MAX;
					}
					break;
				
				case R_RIGHT:
					if (!REVERSE_LR) {
						ReportData->RX = STICK_MAX;
					} else {
						ReportData->RX = STICK_MIN;
					}
					break;
				
				case TOP:
					ReportData->HAT = HAT_TOP;
					break;
				
				case BOTTOM:
					ReportData->HAT = HAT_BOTTOM;
					break;
				
				case LEFT:
					ReportData->HAT = HAT_LEFT;
					break;
				
				case RIGHT:
					ReportData->HAT = HAT_RIGHT;
					break;
				
				case A:
					ReportData->Button |= SWITCH_A;
					break;
				
				case B:
					ReportData->Button |= SWITCH_B;
					break;
				
				case X:
					ReportData->Button |= SWITCH_X;
					break;
				
				case Y:
					ReportData->Button |= SWITCH_Y;
					break;
				
				case L:
					ReportData->Button |= SWITCH_L;
					break;
				
				case R:
					ReportData->Button |= SWITCH_R;
					break;
				
				case ZL:
					ReportData->Button |= SWITCH_ZL;
					break;
				
				case ZR:
					ReportData->Button |= SWITCH_ZR;
					break;
				
				case MINUS:
					ReportData->Button |= SWITCH_MINUS;
					break;
				
				case PLUS:
					ReportData->Button |= SWITCH_PLUS;
					break;
				
				case TRIGGERS:
					ReportData->Button |= SWITCH_L | SWITCH_R;
					break;
				
				case AIM_SHOT:
					ReportData->Button |= SWITCH_ZR;
					if (!REVERSE_UD) {
						ReportData->RY = STICK_CENTER - 36;
					} else {
						ReportData->RY = STICK_CENTER + 36;
					}
					if (!REVERSE_LR) {
						ReportData->RX = STICK_CENTER - 22;
					} else {
						ReportData->RX = STICK_CENTER + 22;
					}					
					break;

				case AIM_MAP:
					ReportData->LX = STICK_MIN;
					ReportData->LY = 192;
					break;
				
				case JUMP:
					ReportData->Button |= SWITCH_B;
					ReportData->LY = STICK_MIN;
					break;

				case END:
					/* 
					if (step == SYNC_CONTROLLER) {
						bufindex = 0;
						duration_count = 0;
						step = ENTER_STAGE;
					}
					// デバッグ用です。
					// 1-8ヤカン上でマイコンを接続すると、感度設定などをスキップして周回を始めます。
					*/
					
					if (INFINITE_LOOP_MODE && step == CLEAR_STAGE) {
						step = ENTER_STAGE;
						bufindex = 0;
						duration_count = 0;
						clear_count = 0;
					}					
					else if (step == CLEAR_STAGE && (0 < clear_count && clear_count < 4)) {
						step = ENTER_STAGE;
						bufindex = 0;
						duration_count = 0;
					}
					else if (step == LUNCH_DRONE) {
						step = OPEN_OPTION;
						bufindex = 0;
						duration_count = 0;
					}
					else if (step == OPEN_OPTION && cnt == 2) {
						step = RESET_SENSITIVITY;
						bufindex = 0;
						duration_count = 0;
					}
					else if (step == SET_SENSITIVITY && flag == 0) {
						bufindex = 0;
						duration_count = 0;
					}
					else if (step == RESET_SENSITIVITY && flag == 0) {
						bufindex = 0;
						duration_count = 0;
					}
					else {
						if (SOFT_TYPE && (step == GO_TO_ALTERNA || step == ENTER_STAGE) && duration_count < tmp.duration) {
							break;
						} else {
							step++;
							bufindex = 0;
							duration_count = 0;
						}
					}

					break;
				
				default:
					ReportData->LX = STICK_CENTER;
					ReportData->LY = STICK_CENTER;
					ReportData->RX = STICK_CENTER;
					ReportData->RY = STICK_CENTER;
					ReportData->HAT = HAT_CENTER;
					break;
			}

			duration_count++;
			
			if (tmp.button != END && duration_count > tmp.duration) {
				bufindex++;
				duration_count = 0;
			}

			break;

		case DONE:
			#ifdef ALERT_WHEN_DONE
			portsval = ~portsval;
			PORTD = portsval; //flash LED(s) and sound buzzer if attached
			PORTB = portsval;
			_delay_ms(250);
			#endif
			return;
	}


	// Prepare to echo this report
	memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	echoes = ECHOES;

}