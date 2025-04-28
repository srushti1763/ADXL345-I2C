#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "sleep.h"

// --------------------- ADXL345 Register Definitions ---------------------
#define ADXL345_ADDR              0x53
#define ADXL345_REG_DEVID         0x00
#define ADXL345_REG_POWER_CTL     0x2D
#define ADXL345_REG_DATA_FORMAT   0x31
#define ADXL345_REG_DATAX0        0x32
#define ADXL345_DATA_LENGTH       6

// --------------------- Device ID ---------------------
#define I2C_DEVICE_ID             XPAR_XIICPS_0_DEVICE_ID  // Change if using I2C1

// --------------------- Global I2C Instance ---------------------
XIicPs I2CInstance;

// --------------------- I2C Initialization ---------------------
int I2C_Init() {
    XIicPs_Config *Config;
    int Status;

    Config = XIicPs_LookupConfig(I2C_DEVICE_ID);
    if (Config == NULL) {
        return XST_FAILURE;
    }

    Status = XIicPs_CfgInitialize(&I2CInstance, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XIicPs_SetSClk(&I2CInstance, 100000);  // Set to 100 kHz

    return XST_SUCCESS;
}

// --------------------- Write 1 Byte to ADXL345 Register ---------------------
int I2C_WriteReg(u8 dev_addr, u8 reg_addr, u8 data) {
    u8 buffer[2];
    buffer[0] = reg_addr;
    buffer[1] = data;

    int Status = XIicPs_MasterSendPolled(&I2CInstance, buffer, 2, dev_addr);
    while (XIicPs_BusIsBusy(&I2CInstance));
    return Status;
}

// --------------------- Read Bytes from ADXL345 Register ---------------------
int I2C_ReadRegs(u8 dev_addr, u8 reg_addr, u8 *data, int len) {
    int Status;

    // Write the register address to ADXL345
    Status = XIicPs_MasterSendPolled(&I2CInstance, &reg_addr, 1, dev_addr);
    while (XIicPs_BusIsBusy(&I2CInstance));
    if (Status != XST_SUCCESS) return Status;

    // Read the data
    Status = XIicPs_MasterRecvPolled(&I2CInstance, data, len, dev_addr);
    while (XIicPs_BusIsBusy(&I2CInstance));

    return Status;
}

// --------------------- ADXL345 Initialization ---------------------
void ADXL345_Init() {
    I2C_WriteReg(ADXL345_ADDR, ADXL345_REG_POWER_CTL, 0x00);  // Wakeup
    usleep(10000);

    I2C_WriteReg(ADXL345_ADDR, ADXL345_REG_POWER_CTL, 0x08);  // Measurement Mode
    usleep(10000);

    I2C_WriteReg(ADXL345_ADDR, ADXL345_REG_DATA_FORMAT, 0x01);  // +/-4g range, right justified
    usleep(10000);
}

// --------------------- Read X, Y, Z Acceleration ---------------------
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z) {
    u8 buf[ADXL345_DATA_LENGTH];

    I2C_ReadRegs(ADXL345_ADDR, ADXL345_REG_DATAX0, buf, ADXL345_DATA_LENGTH);

    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
}

// --------------------- Main Function ---------------------
int main() {
    int Status;
    int16_t x, y, z;
    u8 devid = 0;

    xil_printf("Initializing I2C...\n");

    Status = I2C_Init();
    if (Status != XST_SUCCESS) {
        xil_printf("I2C Initialization Failed!\n");
        return XST_FAILURE;
    }

    // Optional: Check device ID
    I2C_ReadRegs(ADXL345_ADDR, ADXL345_REG_DEVID, &devid, 1);
    xil_printf("ADXL345 Device ID: 0x%02X\r\n", devid);  // Should be 0xE5
    if (devid != 0xE5) {
        xil_printf("Device ID mismatch! Check wiring or I2C address.\r\n");
    }

    ADXL345_Init();
    xil_printf("ADXL345 Initialized. Reading Acceleration:\n");

    while (1) {
        ADXL345_ReadXYZ(&x, &y, &z);
        xil_printf("X: %d, Y: %d, Z: %d\r\n", x, y, z);
        usleep(500000);  // 500 ms
    }

    return 0;
}
