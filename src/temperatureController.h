void initTemperatureController(void);
void setFanPWMbasedOnTemperature(void);
void Set_target_temp(float Value);
float getTargetTemperature(void);
float getActualTemperature(void);
bool Getsavestatus(void);
void SETsavestatus(void);
void setActualTemperatureAndPublishMQTT(float aActualTemperature);
void updatePWM_MQTT_Screen_withNewTargetTemperature(float aTargetTemperature, bool force);
void updatePWM_MQTT_Screen_withNewActualTemperature(float aActualTemperature, bool force);

