set JET= ..\..\..\..\..\Tools\OTAUtils\JET.exe
:: Change the path to the OTA Build folder.
cd ..\..\DimmerSwitch\Build\OTABuild

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::Build Encrpted Client Binary:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Add serialisation Data with ImageType = 0x1XXX - Indicates it is for Encrpted devices
..\..\..\..\..\Tools\OTAUtils\JET.exe -m combine -f DimmerSwitch.bin -x configOTA_6x_Cer_Keys_HA_Switch.txt -v 4 -g 1 -k 0xffffffffffffffffffffffffffffffff -u 0x1037 -t 0x1104

:: Creat an Unencrpted Bootable Client with Veriosn 1
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c outputffffffffffffffff.bin -o Client.bin -v 4 -n 1 -u 0x1037 -t 0x1104

::Creat an Encrypted Image from the Bootable UnEncrpted Client - This must be copied to Ext Flash and then erase internal Flash
::..\..\..\..\..\Tools\OTAUtils\JET.exe -m bin -f Client.bin -e Client_Enc.bin -k 0xffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 -v 4

::::::::::::::::::::::::::::::::::::::::Build OTA Encrypted Upgarde Image from the Unencrypted Bootable Client Image :::::::::::::::::::::::::::::::::::::::::::::::::::
:: Modify Embedded Header to reflect version 2 
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV2.bin -v 4 -n 2 -u 0x1037 -t 0x1104

:: Now Encrypt the above Version 2  
..\..\..\..\..\Tools\OTAUtils\JET.exe -m bin -f UpGradeImagewithOTAHeaderV2.bin -e UpGradeImagewithOTAHeaderV2_Enc.bin -v 4 -k ffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 

:: Wrap the Image with OTA header with version 2
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV2_Enc.bin -o Client_UpgradeImagewithOTAHeaderV2_Enc.bin -v 4 -n 2 -u 0x1037 -t 0x1104


:: Modify Embedded Header to reflect version 3 
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV3.bin -v 4 -n 3 -u 0x1037 -t 0x1104

:: Now Encrypt the above Version 3  
..\..\..\..\..\Tools\OTAUtils\JET.exe -m bin -f UpGradeImagewithOTAHeaderV3.bin -e UpGradeImagewithOTAHeaderV3_Enc.bin -v 4 -k ffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 

:: Wrap the Image with OTA header with version 3
..\..\..\..\..\Tools\OTAUtils\JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV3_Enc.bin -o Client_UpgradeImagewithOTAHeaderV3_Enc.bin -v 4 -n 3 -u 0x1037 -t 0x1104


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Clean Up Imtermediate files:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

::rm Light.bin 
rm output*.bin
rm UpGradeImagewithOTAHeader*.bin

chmod 777 Client.bin
chmod 777 Client_UpgradeImagewithOTAHeaderV2_Enc.bin
chmod 777 Client_UpgradeImagewithOTAHeaderV3_Enc.bin