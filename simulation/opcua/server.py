import asyncio
import os
import random

from asyncua import Server


async def main():
    port = int(os.environ.get("OPCUA_PORT", "4840"))
    path = os.environ.get("OPCUA_PATH", "/freeopcua/server/")
    if not path.startswith("/"):
        path = "/" + path

    server = Server()
    await server.init()
    server.set_endpoint(f"opc.tcp://0.0.0.0:{port}{path}")
    server.set_server_name("Demo OPC UA Simulator")
    print(f"[opcua] endpoint opc.tcp://0.0.0.0:{port}{path}", flush=True)

    uri = "urn:demo:opcua:mqtt"
    idx = await server.register_namespace(uri)

    demo = await server.nodes.objects.add_object(idx, "Demo")

    counter = await demo.add_variable(idx, "Counter", 0)
    temperature = await demo.add_variable(idx, "Temperature", 21.5)
    running = await demo.add_variable(idx, "Running", True)
    machine_state = await demo.add_variable(idx, "MachineState", "IDLE")
    pressure = await demo.add_variable(idx, "Pressure", 1.2)
    setpoint = await demo.add_variable(idx, "Setpoint", 23.0)

    async with server:
        i = 0
        while True:
            i += 1

            is_running = (i % 20) < 14
            state = "RUNNING" if is_running else "IDLE"

            temp = round(20.0 + random.random() * 5.0, 2)
            press = round(1.0 + random.random() * 0.5, 3)
            sp = round(22.0 + ((i // 10) % 3), 1)

            await counter.write_value(i)
            await running.write_value(is_running)
            await machine_state.write_value(state)
            await temperature.write_value(temp)
            await pressure.write_value(press)
            await setpoint.write_value(sp)

            print(
                "[opcua]"
                f" counter={i} temp={temp} running={is_running}"
                f" state={state} pressure={press} setpoint={sp}",
                flush=True,
            )

            await asyncio.sleep(1)


if __name__ == "__main__":
    asyncio.run(main())