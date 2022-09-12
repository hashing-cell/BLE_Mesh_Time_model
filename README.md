This is an implementation of the Bluetooth Mesh Time Models according to the specifications https://www.bluetooth.com/specifications/specs/mesh-model-1-0-1/. 

This model is meant to be used for the nrf5 SDK for Mesh: https://github.com/NordicSemiconductor/nRF5-SDK-for-Mesh

Note, 1 limitation of this is that the Time Server does not use persistent storage and thus its state is not stored. This is left up to the user to store its state into persistent storage and recover upon restart


