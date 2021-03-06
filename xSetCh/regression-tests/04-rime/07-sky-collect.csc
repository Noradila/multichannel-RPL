<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project>../apps/mrm</project>
  <project>../apps/mspsim</project>
  <project>../apps/avrora</project>
  <project>../apps/native_gateway</project>
  <simulation>
    <title>My simulation</title>
    <delaytime>0</delaytime>
    <randomseed>generated</randomseed>
    <motedelay_us>10000000</motedelay_us>
    <radiomedium>
      se.sics.cooja.radiomediums.UDGM
      <transmitting_range>30.0</transmitting_range>
      <interference_range>40.0</interference_range>
      <success_ratio_tx>0.9</success_ratio_tx>
      <success_ratio_rx>0.9</success_ratio_rx>
    </radiomedium>
    <motetype>
      se.sics.cooja.mspmote.SkyMoteType
      <identifier>sky1</identifier>
      <description>Sky Mote Type #1</description>
      <source>[CONTIKI_DIR]/examples/sky/sky-collect.c</source>
      <commands>make clean TARGET=sky
make sky-collect.sky TARGET=sky</commands>
      <firmware>[CONTIKI_DIR]/examples/sky/sky-collect.sky</firmware>
      <moteinterface>se.sics.cooja.interfaces.Position</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyByteRadio</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkySerial</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyLED</moteinterface>
    </motetype>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>9.333811152651393</x>
        <y>89.28114548870677</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>1</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>33.040227185226826</x>
        <y>54.184283361563054</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>2</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>-2.2559922410521516</x>
        <y>50.71648775308175</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>3</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>12.959353575718179</x>
        <y>43.874396471224806</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>4</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>15.917348901177405</x>
        <y>66.93526904376517</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>5</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>26.735174243053933</x>
        <y>35.939375910459084</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>6</id>
      </interface_config>
    </mote>
    <mote>
      se.sics.cooja.mspmote.SkyMote
      <motetype_identifier>sky1</motetype_identifier>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>41.5254792748469</x>
        <y>28.370611308140152</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>7</id>
      </interface_config>
    </mote>
  </simulation>
  <plugin>
    se.sics.cooja.plugins.SimControl
    <width>265</width>
    <z>3</z>
    <height>200</height>
    <location_x>0</location_x>
    <location_y>0</location_y>
    <minimized>false</minimized>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.Visualizer
    <plugin_config>
      <skin>Mote IDs</skin>
      <skin>Radio environment (UDGM)</skin>
    </plugin_config>
    <width>264</width>
    <z>1</z>
    <height>185</height>
    <location_x>0</location_x>
    <location_y>200</location_y>
    <minimized>false</minimized>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>TIMEOUT(300000, log.log("received/node: " + count[1] + " " + count[2] + " " + count[3] + " " + count[4] + " " + count[5] + " " + count[6] + " " + count[7] + "\n"));

/* Conf. */
booted = new Array();
count = new Array();
nrNodes = 7;
nodes_starting = true;
for (i = 1; i &lt;= nrNodes; i++) {
  booted[i] = false;
  count[i] = 0;
}

/* Wait until all nodes have started */
while (nodes_starting) {
  YIELD_THEN_WAIT_UNTIL(msg.startsWith('Starting'));
  
  log.log("Node " + id + " booted\n");
  booted[id] = true;

  for (i = 1; i &lt;= nrNodes; i++) {
    if (!booted[i]) break;
    if (i == nrNodes) nodes_starting = false;
  }
}

/* Create sink */
log.log("All nodes booted, creating sink at node " + id + "\n");
mote.getInterfaces().getButton().clickButton()

while (true) {
  YIELD();

  /* Count sensor data packets */
  source = msg.split(" ")[0];
  count[source]++;
  log.log("Got data from node " + source + ": tot=" + count[source] + "\n");

  /* Fail if any node has transmitted more than 20 packets */
  for (i = 1; i &lt;= nrNodes; i++) {
    if (count[i] &gt; 20) {
      log.log("received/node: " + count[1] + " " + count[2] + " " + count[3] + " " + count[4] + " " + count[5] + " " + count[6] + " " + count[7] + "\n");
      log.testFailed(); /* We are done! */
    }
  }

  /* Wait until we have received data from all nodes */
  for (i = 1; i &lt;= nrNodes; i++) {
    if (count[i] &lt; 5) break;
    if (i == nrNodes) log.testOK();
  }

}</script>
      <active>true</active>
    </plugin_config>
    <width>600</width>
    <z>2</z>
    <height>385</height>
    <location_x>266</location_x>
    <location_y>0</location_y>
    <minimized>false</minimized>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <mote>1</mote>
      <mote>2</mote>
      <mote>3</mote>
      <mote>4</mote>
      <mote>5</mote>
      <mote>6</mote>
      <showRadioRXTX />
      <split>109</split>
      <zoom>9</zoom>
    </plugin_config>
    <width>866</width>
    <z>0</z>
    <height>152</height>
    <location_x>0</location_x>
    <location_y>384</location_y>
    <minimized>false</minimized>
  </plugin>
</simconf>

