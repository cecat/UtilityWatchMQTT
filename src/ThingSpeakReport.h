/*
 * This code is useful if one wants to use webhooks at the Particle Cloud
 * to send data to ThingSpeak.com - it predates the Home Assistant setup but I've
 * kept it as it now has several years of historical data about my appliance behavior
 */
void ThingSpeakReport() {
    // Report to Particle cloud --> webhooks --> ThingSpeak
        int cases = 5;                                  // hard code case # here, feels like a kluge
        switch ((reportCount++) % cases) {              // round robin reporting one var at a time
            case 0:                                     // sump current
                for (int j=0;j<HIST;j++) {              // report the max we've seen in case we missed a short run
                    sumpCur = max (sumpCur, sumpHistory[j]);
                    sumpHistory[j]=0;
                }
                Particle.publish("sumpCurrent", String(sumpCur),    PRIVATE);
                break;
            case 1:                                     // hvac current
                Particle.publish("hvacCurrent", String(hvacCur),    PRIVATE);
                break;
            case 2:                                     // water heater chimney temperature
                Particle.publish("waterTemp",   String(waterTemp),  PRIVATE);
                break;
            case 3:                                     // duration of last hvac event (if there was one; 0 if not)
                if (hvacEvent) {
                    if (!hvacOn) Particle.publish("hvacEvent", String(hvacDuration), PRIVATE);
                    hvacEvent = false;
                } else {
                    Particle.publish("hvacEvent", "0", PRIVATE);
                }
                break;
            case 4:                                     // duration of last sump event (if there was one; 0 if not)
                if (sumpEvent) {
                    Particle.publish("sumpEvent", String(sumpDuration), PRIVATE);
                    sumpEvent = false;
                } else {
                    Particle.publish("sumpEvent", "0", PRIVATE);
                }
                break;
        }
            
}
