set JET= ..\..\..\..\..\Tools\OTAUtils\JET.exe
:: Change the path to the OTA Build folder.
cd ..\..\DimmerSwitch\Build\OTABuild

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::Build Unencrpted Client Binary::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Add serialisation Data with ImageType = 0x0XXX - Indicates it is for Encrpted devices
..\..\..\..\..\Tools\OTAUtils\JET.exe -m combine -f DimmerSwitch.bin -x configOTA_6x_Cer_Keys_HA_Switch.txt -v 4 -g 1 -k 0xffffffffffffffffffffffffffffffff -u 0x1037 -t 0x0104

:: Creat an Unencrpted Bootable Client with Veriosn 1
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c outputffffffffffffffff.bin -o Client.bin -v 4 -n 1 -u 0x1037 -t 0x0104

::::::::::::::::::::::::::::::::::::::::Build OTA Unencrypted Upgarde Image from the Bootable Client  :::::::::::::::::::::::::::::::::::::::::::::::::::
:: Modify Embedded Header to reflect version 2 
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV2.bin -v 4 -n 2 -u 0x1037 -t 0x0104

:: Wrap the Image with OTA header with version 2
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV2.bin -o ClientUpGradeImagewithOTAHeaderV2.bin -v 4 -n 2 -u 0x1037 -t 0x0104

:: Modify Embedded Header to reflect version 3 
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV3.bin -v 4 -n 3 -u 0x1037 -t 0x0104

:: Wrap the Image with OTA header with version 3
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV3.bin -o ClientUpGradeImagewithOTAHeaderV3.bin -v 4 -n 3 -u 0x1037 -t 0x0104

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Clean Up Imtermediate files:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

::rm Light.bin 
rm output*.bin
rm UpGradeImagewithOTAHeader*.bin

chmod 777 Client.bin
chmod 777 ClientUpGradeImagewithOTAHeaderV2.bin
chmod 777 ClientUpGradeImagewithOTAHeaderV3.bin