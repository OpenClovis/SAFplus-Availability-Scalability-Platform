This component will raise and clear an alarm on \Chassis:0\GigeBlade:0\GigePort:0
which is being managed by GigeComp0. This call will be routed from the
alarm client of this test application to the alarm client of GigeComp0.
This component will print the category and severity of the alarm information.
The printing of category and severity is being done as it is an IN_OUT 
parameter. When wrong values of category and severity is supplied deviating
from the one provided during modeling. Upon successful execution of clAlarmRaise
proper values of category and severity would be returned back, the one provided
during modeling.
