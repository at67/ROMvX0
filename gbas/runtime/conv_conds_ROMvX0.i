; these subs should never be linked, let alone called for ROMvX0

%SUB                convertEqOp
                    ; convert equal to into a boolean
convertEqOp         TEQ     giga_vAC
                    RET
%ENDS   
                    
%SUB                convertNeOp
                    ; convert not equal to into a boolean
convertNeOp         TNE     giga_vAC
                    RET
%ENDS   
    
%SUB                convertLeOp                
                    ; convert less than or equal to into a boolean
convertLeOp         TLE     giga_vAC
                    RET
%ENDS   
    
%SUB                convertGeOp
                    ; convert greater than or equal to into a boolean
convertGeOp         TGE     giga_vAC
                    RET
%ENDS   
        
%SUB                convertLtOp     
                    ; convert less than into a boolean
convertLtOp         TLT     giga_vAC
                    RET
%ENDS   
        
%SUB                convertGtOp     
                    ; convert greater than into boolean
convertGtOp         TGT     giga_vAC
                    RET
%ENDS