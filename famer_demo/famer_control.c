#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>

#include "hi_i2c.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_errno.h"
#include "hi_time.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "iot_gpio_ex.h"
#include "iot_errno.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "aht20.h"
#include "BH1750.h"
#include "oled_ssd1306.h"
#include "sg92r.h"
#include "SGP30.h"
#include "E53_IA1.h"
#include "wifi_connect.h"


#define STACK_SIZE 4096
#define I2C_DATA_RATE 400000
#define IOT_GPIO_IDX_0 0
#define IOT_GPIO_IDX_1 1
#define IOT_I2C_IDX_1 1
#define IDX_0          0
#define IDX_1          1
#define IDX_2          2
#define IDX_3          3
#define IDX_4          4
#define IDX_5          5
#define IDX_6          6
#define IOT_PWM_PORT_PWM0   0
#define CONFIG_WIFI_SSID "BearPi" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "123456789" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "121.36.42.100"

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "64b758afff796022370ae681_20211071130" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "20211071130" // 替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME 60 // seconds

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)

#define MSGQUEUE_COUNT 16
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24
#define SENSOR_TASK_STACK_SIZE (1024 * 4)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY 10

osMessageQueueId_t mid_MsgQueue; // message queue id
typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
} en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    float lum;
    float temp;
    float hum;
} report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    osMessageQueueId_t app_msg;
    int connected;
    int led;
    int motor;
} app_cb_t;
static app_cb_t g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t humidity;
    oc_mqtt_profile_kv_t luminance;
    oc_mqtt_profile_kv_t led;
    oc_mqtt_profile_kv_t motor;

    // if (g_app_cb.connected != 1)
    // {
    //     return;
    // }

    service.event_time = NULL;
    service.service_id = "Agriculture";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    temperature.nxt = &humidity;

    humidity.key = "Humidity";
    humidity.value = &report->hum;
    humidity.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    humidity.nxt = &luminance;

    luminance.key = "Luminance";
    luminance.value = &report->lum;
    luminance.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    luminance.nxt = &led;

    led.key = "DoorStatus";
    if(g_app_cb.led ==1)
    {led.value = "ON";}
    else
    {led.value = "OFF";;}
    led.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    led.nxt = &motor;

    motor.key = "MotorStatus";
    if(g_app_cb.motor ==1)
    {motor.value = "ON";}
    else
    {motor.value = "OFF";}
    motor.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motor.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}
// use this function to push all the message to the buffer
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int ret = 0;
    char *buf;
    int buf_len;
    app_msg_t *app_msg;

    if ((msg == NULL) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS))
    {
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if (buf == NULL)
    {
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy_s(app_msg->msg.cmd.request_id, buf_len, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy_s(app_msg->msg.cmd.payload, buf_len, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT);
    if (ret != 0)
    {
        free(app_msg);
    }

    return ret;
}

static void oc_cmdresp(cmd_t *cmd, int cmdret)
{
    oc_mqtt_profile_cmdresp_t cmdresp;
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_light_cmd(cmd_t *cmd, cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "paras");
    if (obj_paras == NULL)
    {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "Door");
    if (obj_paras == NULL)
    {
        cJSON_Delete(obj_root);
    }
    ///< operate the LED here
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0)
    {
        g_app_cb.led = 1;
        Engine3TurnLeft_90();
        printf("Door On!\r\n");
    }
    else
    {
        g_app_cb.led = 0;
        Engine3RegressMiddle();
        printf("Door Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}

static void deal_motor_cmd(cmd_t *cmd, cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
    if (obj_paras == NULL)
    {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "Motor");
    if (obj_para == NULL)
    {
        cJSON_Delete(obj_root);
    }
    ///< operate the Motor here
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0)
    {
        g_app_cb.motor = 1;
        motor_forward();
        printf("Motor On!\r\n");
    }
    else
    {
        g_app_cb.motor = 0;
        motor_stop();
        printf("Motor Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}

static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;

    int cmdret = 1;
    obj_root = cJSON_Parse(cmd->payload);
    if (obj_root == NULL)
    {
        oc_cmdresp(cmd, cmdret);
    }
    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (obj_cmdname == NULL)
    {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Door") == 0)
    {
        deal_light_cmd(cmd, obj_root);
    }
    else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor") == 0)
    {
        deal_motor_cmd(cmd, obj_root);
    }

    return;
}

void AHT20_I2C_Init(void)
{
    IoTGpioInit(IOT_GPIO_IDX_0);
    hi_io_set_func(HI_IO_NAME_GPIO_0, HI_IO_FUNC_GPIO_0_I2C1_SDA);
    IoTGpioInit(IOT_GPIO_IDX_1);
    hi_io_set_func(HI_IO_NAME_GPIO_1, HI_IO_FUNC_GPIO_1_I2C1_SCL);
    IoTI2cInit(IOT_I2C_IDX_1,I2C_DATA_RATE);
    IoTI2cSetBaudrate(IOT_I2C_IDX_1,400000);
}


static int CloudMainTaskEntry(void)
{
    app_msg_t *app_msg;
    uint32_t ret;

    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (g_app_cb.app_msg == NULL)
    {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok))
    {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }

    while (1)
    {
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
        if (app_msg != NULL)
        {
            switch (app_msg->msg_type)
            {
            case en_msg_cmd:
                deal_cmd_msg(&app_msg->msg.cmd);
                break;
            case en_msg_report:
                deal_report_msg(&app_msg->msg.report);
                break;
            default:
                break;
            }
            free(app_msg);
        }
    }
    return 0;
}


static void SensorTaskEntry(void)
{
    int cnt=0;
    float lux;
    int ret=0;
    uint32_t retval=0;
    float humidity = 0.0f;
    float temperature = 0.0f;
    float CO2Data;
    static char line[32] = {0};
    AHT20_I2C_Init();
    BH1750_I2C_Init();
    BH1750_Init();
    usleep(180000);
    //SGP30_Init();
    motor_Init();
    SG92RInit();
    OledInit();
    OledFillScreen(0);
    app_msg_t *app_msg;
    // int res;
    // res = E53IA1Init();
    //if (res != 0)
    //{
       // printf("E53_IA1 Init failed!\r\n");
        //        return;
    //}
   

   while(1)
   {

    while (IOT_SUCCESS != AHT20_Calibrate()) {
        printf("AHT20 sensor init failed!\r\n");
        usleep(1000);
    }
  
    retval = AHT20_StartMeasure();
    if (retval != IOT_SUCCESS) {
        printf("trigger measure failed!\r\n");
    }

    retval = AHT20_GetMeasureResult(&temperature, &humidity);
    if (retval != IOT_SUCCESS) {
        printf("get humidity data failed!\r\n");
    }
    ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "temp: %.2f", temperature);
    if (ret < 0) {
        continue;
    }
    OledShowString(0, IDX_0, line, 1);
    printf("temp:%.02f\r\n",temperature);

    ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "humi: %.2f", humidity);
    if (ret < 0) {
        continue;
    }
    OledShowString(0, IDX_1, line, 1);
    printf("humi:%.02f\r\n",humidity);

    printf("test cnt: %d", cnt++);
    lux=BH1750_ReadLightIntensity();
    printf("sensor val: %.02f [Lux]\n", lux);
    usleep(1000);
    ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "lux: %.2f", lux);
    if (ret < 0) {
        continue;
    }
    OledShowString(0,IDX_2,line,1);
    //CO2Data=SGP30_GetCO2();
    // ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "CO2: %.2f", CO2Data);
    //if (ret < 0) {
        //continue;
    //}
    //OledShowString(0,IDX_3,line,1);
    //printf("CO2Data:%.02f\r\n",CO2Data);

    if(temperature>=30.00)
    {
        Engine1TurnRight_90();
    }
    else
    {
        Engine1RegressMiddle();
    }

    if(humidity<=70.00)
    {
        motor_forward();
    }
    else
    {
        motor_stop();
    }

    if(lux>=1000.00)
    {
        Engine2TurnRight_90();
    }
    else
    {
        Engine2RegressMiddle();
    }
     // res = E53IA1ReadData(&data);
        // if (res != 0)
        // {
        //     printf("E53_IA1 Read Data failed!\r\n");
        //     return;
        // }
        app_msg = malloc(sizeof(app_msg_t));
        // printf("SENSOR:lum:%.2f temp:%.2f hum:%.2f\r\n", data.Lux, data.Temperature, data.Humidity);
        // printf("SENSOR:lum:100 temp:10 hum:1000\r\n");

        app_msg->msg_type = en_msg_report;
        app_msg->msg.report.hum = humidity;
        app_msg->msg.report.lum = lux;
        printf("[Lux]: %.02f \n",app_msg->msg.report.lum);
        app_msg->msg.report.temp = temperature;
        if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT != 0))
        {
            printf("F");
        }
        free(app_msg);
        usleep(TASK_DELAY);
   }
   return 0;
}

static void IotMainTaskEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL)
    {
        printf("Failed to create CloudMainTaskEntry!\n");
    }
    attr.stack_size = SENSOR_TASK_STACK_SIZE;
    attr.priority = SENSOR_TASK_PRIO;
    attr.name = "SensorTaskEntry";
    if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL)
    {
        printf("Failed to create SensorTaskEntry!\n");
    }
}

APP_FEATURE_INIT(IotMainTaskEntry);
   
