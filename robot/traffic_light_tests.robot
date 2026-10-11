*** Settings ***
Library    uart_library.py

Suite Setup       Open Uart    /dev/cu.usbmodem0010501670471    115200
Suite Teardown    Close Uart


*** Test Cases ***

Valid Time String
    ${response}=    Send Command    000120
    Should Be Equal    ${response}    TIME:80

Invalid Time String
    ${response}=    Send Command    001067
    Should Be Equal    ${response}    TIME_ERROR:-3

Seconds Boundary Valid
    ${response}=    Send Command    000059
    Should Be Equal    ${response}    TIME:59

Seconds Boundary Invalid
    ${response}=    Send Command    000060
    Should Be Equal    ${response}    TIME_ERROR:-3

Minutes Boundary Valid
    ${response}=    Send Command    005901
    Should Be Equal    ${response}    TIME:3541

Minutes Boundary Invalid
    ${response}=    Send Command    006001
    Should Be Equal    ${response}    TIME_ERROR:-3

Non Zero Time Valid
    ${response}=    Send Command    000010
    Should Be Equal    ${response}    TIME:10

Zero Time Invalid
    ${response}=    Send Command    000000
    Should Be Equal    ${response}    TIME_ERROR:-5

Valid Traffic Light Sequence
    ${response}=    Send Sequence    RYG
    Should Be Equal    ${response}    SEQUENCE:OK

Invalid Traffic Light Sequence
    ${response}=    Send Sequence    RjG
    Should Be Equal    ${response}    SEQUENCE_ERROR:-12

Debug Sequence Time
    ${response}=    Get Debug Time
    Log    ${response}