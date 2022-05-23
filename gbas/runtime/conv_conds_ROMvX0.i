; these subs should never be linked, let alone called for ROMvX0

%SUB                convertEqOp
                    ; convert equal to into a boolean
convertEqOp         TEQ
                    RET
%ENDS   
                    
%SUB                convertNeOp
                    ; convert not equal to into a boolean
convertNeOp         TNE
                    RET
%ENDS   
    
%SUB                convertLeOp                
                    ; convert less than or equal to into a boolean
convertLeOp         TLE
                    RET
%ENDS   
    
%SUB                convertGeOp
                    ; convert greater than or equal to into a boolean
convertGeOp         TGE
                    RET
%ENDS   
        
%SUB                convertLtOp     
                    ; convert less than into a boolean
convertLtOp         TLT
                    RET
%ENDS   
        
%SUB                convertGtOp     
                    ; convert greater than into boolean
convertGtOp         TGT
                    RET
%ENDS