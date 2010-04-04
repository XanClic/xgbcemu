format ELF

public inc_a
public dec_a
public inc_b
public dec_b
public inc_c
public dec_c
public inc_d
public dec_d
public inc_e
public dec_e
public inc_h
public dec_h
public inc_l
public dec_l
public add_a_b
public add_a_c
public add_a_d
public add_a_e
public add_a_h
public add_a_l
public add_a__hl
public add_a_s
public adc_a_a
public adc_a_b
public adc_a_c
public adc_a_d
public adc_a_e
public adc_a_h
public adc_a_l
public adc_a__hl
public adc_a_s
public add_hl_bc
public add_hl_de
public add_hl_sp
public add_sp_s
public sub_a_b
public sub_a_c
public sub_a_d
public sub_a_e
public sub_a_h
public sub_a_l
public sub_a__hl
public sub_a_s
public sbc_a_b
public sbc_a_c
public sbc_a_d
public sbc_a_e
public sbc_a_h
public sbc_a_l
public sbc_a__hl
public sbc_a_s
public cp_a_b
public cp_a_c
public cp_a_d
public cp_a_e
public cp_a_h
public cp_a_l
public cp_a__hl
public cp_s
public ld_hl_spn
public rlca
public rrca
public rla
public rra

extrn mem_readb

extrn _af
extrn _bc
extrn _de
extrn _hl
extrn _sp
extrn _ip

af = _af
bc = _bc
de = _de
hl = _hl

f = af
a = af + 1
c = bc
b = bc + 1
e = de
d = de + 1
l = hl
h = hl + 1

FLAG_ZERO = 0x80
FLAG_SUB  = 0x40
FLAG_HCRY = 0x20
FLAG_CRY  = 0x10


macro load_flags
{
    mov   dl,[f]
}

macro write_flags
{
    mov   [f],dl
}

macro save_eflags
{
    pushfd
    pop   eax
}

macro check_zf name
{
    jnz   name#_nz
    or    dl,FLAG_ZERO
    name#_nz:
}

macro check_hf name
{
    test  al,0x10
    jz    name#_nh
    or    dl,FLAG_HCRY
    name#_nh:
}

macro check_cf name
{
    jnc   name#_nc
    or    dl,FLAG_CRY
    name#_nc:
}

macro check_cfs name
{
    test  al,0x01
    jz    name#_nc
    or    dl,FLAG_CRY
    name#_nc:
}


inc_a:
load_flags
and   dl,FLAG_CRY
add   byte [a],1
save_eflags
check_zf inc_a
check_hf inc_a
write_flags
ret


dec_a:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [a],1
save_eflags
check_zf dec_a
check_hf dec_a
write_flags
ret


inc_b:
load_flags
and   dl,FLAG_CRY
add   byte [b],1
save_eflags
check_zf inc_b
check_hf inc_b
write_flags
ret


dec_b:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [b],1
save_eflags
check_zf dec_b
check_hf dec_b
write_flags
ret


inc_c:
load_flags
and   dl,FLAG_CRY
add   byte [c],1
save_eflags
check_zf inc_c
check_hf inc_c
write_flags
ret


dec_c:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [c],1
save_eflags
check_zf dec_c
check_hf dec_c
write_flags
ret


inc_d:
load_flags
and   dl,FLAG_CRY
add   byte [d],1
save_eflags
check_zf inc_d
check_hf inc_d
write_flags
ret


dec_d:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [d],1
save_eflags
check_zf dec_d
check_hf dec_d
write_flags
ret


inc_e:
load_flags
and   dl,FLAG_CRY
add   byte [e],1
save_eflags
check_zf inc_e
check_hf inc_e
write_flags
ret


dec_e:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [e],1
save_eflags
check_zf dec_e
check_hf dec_e
write_flags
ret


inc_h:
load_flags
and   dl,FLAG_CRY
add   byte [h],1
save_eflags
check_zf inc_h
check_hf inc_h
write_flags
ret


dec_h:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [h],1
save_eflags
check_zf dec_h
check_hf dec_h
write_flags
ret


inc_l:
load_flags
and   dl,FLAG_CRY
add   byte [l],1
save_eflags
check_zf inc_l
check_hf inc_l
write_flags
ret


dec_l:
load_flags
and   dl,FLAG_CRY
or    dl,FLAG_SUB
sub   byte [l],1
save_eflags
check_zf dec_l
check_hf dec_l
write_flags
ret


add_a_b:
xor   dl,dl
mov   al,[b]
add   [a],al
save_eflags
check_zf add_a_b
check_hf add_a_b
check_cfs add_a_b
write_flags
ret


add_a_c:
xor   dl,dl
mov   al,[c]
add   [a],al
save_eflags
check_zf add_a_c
check_hf add_a_c
check_cfs add_a_c
write_flags
ret


add_a_d:
xor   dl,dl
mov   al,[d]
add   [a],al
save_eflags
check_zf add_a_d
check_hf add_a_d
check_cfs add_a_d
write_flags
ret


add_a_e:
xor   dl,dl
mov   al,[e]
add   [a],al
save_eflags
check_zf add_a_e
check_hf add_a_e
check_cfs add_a_e
write_flags
ret


add_a_h:
xor   dl,dl
mov   al,[h]
add   [a],al
save_eflags
check_zf add_a_h
check_hf add_a_h
check_cfs add_a_h
write_flags
ret


add_a_l:
xor   dl,dl
mov   al,[l]
add   [a],al
save_eflags
check_zf add_a_l
check_hf add_a_l
check_cfs add_a_l
write_flags
ret


add_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
add   [a],al
save_eflags
check_zf add_a__hl
check_hf add_a__hl
check_cfs add_a__hl
write_flags
ret


add_a_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
add   [a],al
save_eflags
check_zf add_a_s
check_hf add_a_s
check_cfs add_a_s
write_flags
add   word [_ip],1
ret


adc_a_a:
xor   dl,dl
mov   al,[a]
clc
test  byte [f],FLAG_CRY
jz    adc_a_a_ncb
stc
adc_a_a_ncb:
adc   [a],al
save_eflags
check_zf adc_a_a
check_hf adc_a_a
check_cfs adc_a_a
write_flags
ret


adc_a_b:
xor   dl,dl
mov   al,[b]
clc
test  byte [f],FLAG_CRY
jz    adc_a_b_ncb
stc
adc_a_b_ncb:
adc   [a],al
save_eflags
check_zf adc_a_b
check_hf adc_a_b
check_cfs adc_a_b
write_flags
ret


adc_a_c:
xor   dl,dl
mov   al,[c]
clc
test  byte [f],FLAG_CRY
jz    adc_a_c_ncb
stc
adc_a_c_ncb:
adc   [a],al
save_eflags
check_zf adc_a_c
check_hf adc_a_c
check_cfs adc_a_c
write_flags
ret


adc_a_d:
xor   dl,dl
mov   al,[d]
clc
test  byte [f],FLAG_CRY
jz    adc_a_d_ncb
stc
adc_a_d_ncb:
adc   [a],al
save_eflags
check_zf adc_a_d
check_hf adc_a_d
check_cfs adc_a_d
write_flags
ret


adc_a_e:
xor   dl,dl
mov   al,[e]
clc
test  byte [f],FLAG_CRY
jz    adc_a_e_ncb
stc
adc_a_e_ncb:
adc   [a],al
save_eflags
check_zf adc_a_e
check_hf adc_a_e
check_cfs adc_a_e
write_flags
ret


adc_a_h:
xor   dl,dl
mov   al,[h]
clc
test  byte [f],FLAG_CRY
jz    adc_a_h_ncb
stc
adc_a_h_ncb:
adc   [a],al
save_eflags
check_zf adc_a_h
check_hf adc_a_h
check_cfs adc_a_h
write_flags
ret


adc_a_l:
xor   dl,dl
mov   al,[l]
clc
test  byte [f],FLAG_CRY
jz    adc_a_l_ncb
stc
adc_a_l_ncb:
adc   [a],al
save_eflags
check_zf adc_a_l
check_hf adc_a_l
check_cfs adc_a_l
write_flags
ret


adc_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
clc
test  byte [f],FLAG_CRY
jz    adc_a__hl_ncb
stc
adc_a__hl_ncb:
adc   [a],al
save_eflags
check_zf adc_a__hl
check_hf adc_a__hl
check_cfs adc_a__hl
write_flags
ret


adc_a_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
clc
test  byte [f],FLAG_CRY
jz    adc_a_s_ncb
stc
adc_a_s_ncb:
adc   [a],al
save_eflags
check_zf adc_a_s
check_hf adc_a_s
check_cfs adc_a_s
write_flags
add   word [_ip],1
ret


add_hl_bc:
load_flags
and   dl,FLAG_ZERO
mov   ax,[bc]
add   [l],al
adc   [h],ah
save_eflags
check_cf add_hl_bc
check_hf add_hl_bc
write_flags
ret


add_hl_de:
load_flags
and   dl,FLAG_ZERO
mov   ax,[de]
add   [l],al
adc   [h],ah
save_eflags
check_cf add_hl_de
check_hf add_hl_de
write_flags
ret


add_hl_sp:
load_flags
and   dl,FLAG_ZERO
mov   ax,[_sp]
add   [l],al
adc   [h],ah
save_eflags
check_cf add_hl_sp
check_hf add_hl_sp
write_flags
ret


add_sp_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
cbw
add   [_sp],ax
save_eflags
check_cf add_sp_s
check_hf add_sp_s
write_flags
ret


sub_a_b:
mov   dl,FLAG_SUB
mov   al,[b]
sub   [a],al
save_eflags
check_zf sub_a_b
check_hf sub_a_b
check_cfs sub_a_b
write_flags
ret


sub_a_c:
mov   dl,FLAG_SUB
mov   al,[c]
sub   [a],al
save_eflags
check_zf sub_a_c
check_hf sub_a_c
check_cfs sub_a_c
write_flags
ret


sub_a_d:
mov   dl,FLAG_SUB
mov   al,[d]
sub   [a],al
save_eflags
check_zf sub_a_d
check_hf sub_a_d
check_cfs sub_a_d
write_flags
ret


sub_a_e:
mov   dl,FLAG_SUB
mov   al,[e]
sub   [a],al
save_eflags
check_zf sub_a_e
check_hf sub_a_e
check_cfs sub_a_e
write_flags
ret


sub_a_h:
mov   dl,FLAG_SUB
mov   al,[h]
sub   [a],al
save_eflags
check_zf sub_a_h
check_hf sub_a_h
check_cfs sub_a_h
write_flags
ret


sub_a_l:
mov   dl,FLAG_SUB
mov   al,[l]
sub   [a],al
save_eflags
check_zf sub_a_l
check_hf sub_a_l
check_cfs sub_a_l
write_flags
ret


sub_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
mov   dl,FLAG_SUB
sub   [a],al
save_eflags
check_zf sub_a__hl
check_hf sub_a__hl
check_cfs sub_a__hl
write_flags
ret


sub_a_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
mov   dl,FLAG_SUB
sub   [a],al
save_eflags
check_zf sub_a_s
check_hf sub_a_s
check_cfs sub_a_s
write_flags
add   word [_ip],1
ret


sbc_a_b:
xor   dl,dl
mov   al,[b]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_b_ncb
stc
sbc_a_b_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_b
check_hf sbc_a_b
check_cfs sbc_a_b
write_flags
ret


sbc_a_c:
xor   dl,dl
mov   al,[c]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_c_ncb
stc
sbc_a_c_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_c
check_hf sbc_a_c
check_cfs sbc_a_c
write_flags
ret


sbc_a_d:
xor   dl,dl
mov   al,[d]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_d_ncb
stc
sbc_a_d_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_d
check_hf sbc_a_d
check_cfs sbc_a_d
write_flags
ret


sbc_a_e:
xor   dl,dl
mov   al,[e]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_e_ncb
stc
sbc_a_e_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_e
check_hf sbc_a_e
check_cfs sbc_a_e
write_flags
ret


sbc_a_h:
xor   dl,dl
mov   al,[h]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_h_ncb
stc
sbc_a_h_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_h
check_hf sbc_a_h
check_cfs sbc_a_h
write_flags
ret


sbc_a_l:
xor   dl,dl
mov   al,[l]
clc
test  byte [f],FLAG_CRY
jz    sbc_a_l_ncb
stc
sbc_a_l_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_l
check_hf sbc_a_l
check_cfs sbc_a_l
write_flags
ret


sbc_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
clc
test  byte [f],FLAG_CRY
jz    sbc_a__hl_ncb
stc
sbc_a__hl_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a__hl
check_hf sbc_a__hl
check_cfs sbc_a__hl
write_flags
ret


sbc_a_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
clc
test  byte [f],FLAG_CRY
jz    sbc_a_s_ncb
stc
sbc_a_s_ncb:
sbb   [a],al
save_eflags
check_zf sbc_a_s
check_hf sbc_a_s
check_cfs sbc_a_s
write_flags
add   word [_ip],1
ret


cp_a_b:
mov   dl,FLAG_SUB
mov   al,[b]
cmp   [a],al
save_eflags
check_zf cp_a_b
check_hf cp_a_b
check_cfs cp_a_b
write_flags
ret


cp_a_c:
mov   dl,FLAG_SUB
mov   al,[c]
cmp   [a],al
save_eflags
check_zf cp_a_c
check_hf cp_a_c
check_cfs cp_a_c
write_flags
ret


cp_a_d:
mov   dl,FLAG_SUB
mov   al,[d]
cmp   [a],al
save_eflags
check_zf cp_a_d
check_hf cp_a_d
check_cfs cp_a_d
write_flags
ret


cp_a_e:
mov   dl,FLAG_SUB
mov   al,[e]
cmp   [a],al
save_eflags
check_zf cp_a_e
check_hf cp_a_e
check_cfs cp_a_e
write_flags
ret


cp_a_h:
mov   dl,FLAG_SUB
mov   al,[h]
cmp   [a],al
save_eflags
check_zf cp_a_h
check_hf cp_a_h
check_cfs cp_a_h
write_flags
ret


cp_a_l:
mov   dl,FLAG_SUB
mov   al,[l]
cmp   [a],al
save_eflags
check_zf cp_a_l
check_hf cp_a_l
check_cfs cp_a_l
write_flags
ret


cp_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
mov   dl,FLAG_SUB
cmp   [a],al
save_eflags
check_zf cp_a__hl
check_hf cp_a__hl
check_cfs cp_a__hl
write_flags
ret


cp_s:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
mov   dl,FLAG_SUB
cmp   [a],al
save_eflags
check_zf cp_s
check_hf cp_s
check_cfs cp_s
write_flags
add   word [_ip],1
ret


ld_hl_spn:
xor   eax,eax
mov   ax,[_ip]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
cbw
mov   cx,[_sp]
add   cx,ax
save_eflags
check_cf ld_hl_spn
check_hf ld_hl_spn
mov   [hl],cx
write_flags
add   word [_ip],1
ret


rlca:
xor   dl,dl
mov   al,[a]
rol   al,1
mov   [a],al
check_cf rlca
test  al,al
check_zf rlca
write_flags
ret


rrca:
xor   dl,dl
mov   al,[a]
ror   al,1
mov   [a],al
check_cf rrca
test  al,al
check_zf rrca
write_flags
ret


rla:
xor   dl,dl
mov   al,[a]
rcl   al,1
mov   [a],al
check_cf rla
test  al,al
check_zf rla
write_flags
ret


rra:
xor   dl,dl
mov   al,[a]
rcr   al,1
mov   [a],al
check_cf rra
test  al,al
check_zf rra
write_flags
ret
