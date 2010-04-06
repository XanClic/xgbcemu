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
public cp_a_s
public ld_hl_spn
public rlca
public rlc_a
public rlc_b
public rlc_c
public rlc_d
public rlc_e
public rlc_h
public rlc_l
public rlc__hl
public rrca
public rrc_a
public rrc_b
public rrc_c
public rrc_d
public rrc_e
public rrc_h
public rrc_l
public rrc__hl
public rla
public rl_a
public rl_b
public rl_c
public rl_d
public rl_e
public rl_h
public rl_l
public rl__hl
public rra
public rr_a
public rr_b
public rr_c
public rr_d
public rr_e
public rr_h
public rr_l
public rr__hl
public sla_a
public sla_b
public sla_c
public sla_d
public sla_e
public sla_h
public sla_l
public sla__hl
public sra_a
public sra_b
public sra_c
public sra_d
public sra_e
public sra_h
public sra_l
public sra__hl
public srl_a
public srl_b
public srl_c
public srl_d
public srl_e
public srl_h
public srl_l
public srl__hl
public swap_a
public swap_b
public swap_c
public swap_d
public swap_e
public swap_h
public swap_l
public swap__hl

extrn mem_readb
extrn mem_writeb

extrn r_af
extrn r_bc
extrn r_de
extrn r_hl
extrn r_sp
extrn r_ip

 af = r_af
 bc = r_bc
 de = r_de
 hl = r_hl
_sp = r_sp
_ip = r_ip

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

BIT_CRY   = 4


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


macro _inc reg
{
    inc_#reg#:
    load_flags
    and   dl,FLAG_CRY
    add   byte [reg],1
    save_eflags
    check_zf inc_#reg
    check_hf inc_#reg
    write_flags
    ret
}

_inc a
_inc b
_inc c
_inc d
_inc e
_inc h
_inc l


macro _dec reg
{
    dec_#reg#:
    load_flags
    and   dl,FLAG_CRY
    sub   byte [reg],1
    save_eflags
    check_zf dec_#reg
    check_hf dec_#reg
    write_flags
    ret
}

_dec a
_dec b
_dec c
_dec d
_dec e
_dec h
_dec l


macro _add_a reg
{
    add_a_#reg#:
    xor   dl,dl
    mov   al,[reg]
    add   [a],al
    save_eflags
    check_zf add_a_#reg
    check_hf add_a_#reg
    check_cfs add_a_#reg
    write_flags
    ret
}

_add_a b
_add_a c
_add_a d
_add_a e
_add_a h
_add_a l

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


macro _adc_a reg
{
    adc_a_#reg#:
    xor   dl,dl
    mov   al,[reg]
    bt    dword [f],BIT_CRY
    adc   [a],al
    save_eflags
    check_zf adc_a_#reg
    check_hf adc_a_#reg
    check_cfs adc_a_#reg
    write_flags
    ret
}

_adc_a a
_adc_a b
_adc_a c
_adc_a d
_adc_a e
_adc_a h
_adc_a l

adc_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
bt    dword [f],BIT_CRY
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


macro _add_hl reg,asreg
{
    add_hl_#asreg#:
    load_flags
    and   dl,FLAG_ZERO
    mov   ax,[reg]
    add   [l],al
    adc   [h],ah
    save_eflags
    check_cf add_hl_#asreg
    check_hf add_hl_#asreg
    write_flags
    ret
}

_add_hl bc,bc
_add_hl de,de
_add_hl _sp,sp


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


macro _sub_a reg
{
    sub_a_#reg#:
    mov   dl,FLAG_SUB
    mov   al,[reg]
    sub   [a],al
    save_eflags
    check_zf sub_a_#reg
    check_hf sub_a_#reg
    check_cfs sub_a_#reg
    write_flags
    ret
}

_sub_a b
_sub_a c
_sub_a d
_sub_a e
_sub_a h
_sub_a l

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


macro _sbc_a reg
{
    sbc_a_#reg#:
    xor   dl,dl
    mov   al,[reg]
    bt    dword [f],BIT_CRY
    sbb   [a],al
    save_eflags
    check_zf sbc_a_#reg
    check_hf sbc_a_#reg
    check_cfs sbc_a_#reg
    write_flags
    ret
}

_sbc_a b
_sbc_a c
_sbc_a d
_sbc_a e
_sbc_a h
_sbc_a l

sbc_a__hl:
xor   eax,eax
mov   ax,[hl]
push  eax
call  mem_readb
add   esp,4
xor   dl,dl
bt    dword [f],BIT_CRY
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


macro _cp_a reg
{
    cp_a_#reg#:
    mov   dl,FLAG_SUB
    mov   al,[reg]
    cmp   [a],al
    save_eflags
    check_zf cp_a_#reg
    check_hf cp_a_#reg
    check_cfs cp_a_#reg
    write_flags
    ret
}

_cp_a b
_cp_a c
_cp_a d
_cp_a e
_cp_a h
_cp_a l

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

cp_a_s:
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


macro _rlc reg
{
    rlc_#reg#:
    xor   dl,dl
    mov   al,[reg]
    rol   al,1
    mov   [reg],al
    check_cf rlc_#reg
    test  al,al
    check_zf rlc_#reg
    write_flags
    ret
}

rlca:
_rlc a
_rlc b
_rlc c
_rlc d
_rlc e
_rlc h
_rlc l

rlc__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
rol   al,1
check_cf rlc__hl
test  al,al
check_zf rlc__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _rrc reg
{
    rrc_#reg#:
    xor   dl,dl
    mov   al,[reg]
    ror   al,1
    mov   [reg],al
    check_cf rrc_#reg
    test  al,al
    check_zf rrc_#reg
    write_flags
    ret
}

rrca:
_rrc a
_rrc b
_rrc c
_rrc d
_rrc e
_rrc h
_rrc l

rrc__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
ror   al,1
check_cf rrc__hl
test  al,al
check_zf rrc__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _rl reg
{
    rl_#reg#:
    xor   dl,dl
    mov   al,[reg]
    bt    dword [f],BIT_CRY
    rcl   al,1
    mov   [reg],al
    check_cf rl_#reg
    test   al,al
    check_zf rl_#reg
    write_flags
    ret
}

rla:
_rl a
_rl b
_rl c
_rl d
_rl e
_rl h
_rl l

rl__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
bt    dword [f],BIT_CRY
rcl   al,1
check_cf rl__hl
test  al,al
check_zf rl__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _rr reg
{
    rr_#reg#:
    xor   dl,dl
    mov   al,[reg]
    bt    dword [f],BIT_CRY
    rcr   al,1
    mov   [reg],al
    check_cf rr_#reg
    test   al,al
    check_zf rr_#reg
    write_flags
    ret
}

rra:
_rr a
_rr b
_rr c
_rr d
_rr e
_rr h
_rr l

rr__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
bt    dword [f],BIT_CRY
check_cf rr__hl
test  al,al
check_zf rr__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _sla reg
{
    sla_#reg#:
    xor   dl,dl
    sal   byte [reg],1
    save_eflags
    check_zf sla_#reg
    check_cfs sla_#reg
    write_flags
    ret
}

_sla a
_sla b
_sla c
_sla d
_sla e
_sla h
_sla l

sla__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
sal   al,1
save_eflags
check_zf sla__hl
check_cfs sla__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _sra reg
{
    sra_#reg#:
    xor   dl,dl
    sar   byte [reg],1
    save_eflags
    check_zf sra_#reg
    check_cfs sra_#reg
    write_flags
    ret
}

_sra a
_sra b
_sra c
_sra d
_sra e
_sra h
_sra l

sra__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
sar   al,1
save_eflags
check_zf sra__hl
check_cfs sra__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _srl reg
{
    srl_#reg#:
    xor   dl,dl
    shr   byte [reg],1
    save_eflags
    check_zf srl_#reg
    check_cfs srl_#reg
    write_flags
    ret
}

_srl a
_srl b
_srl c
_srl d
_srl e
_srl h
_srl l

srl__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
shr   al,1
save_eflags
check_zf srl__hl
check_cfs srl__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret


macro _swap reg
{
    swap_#reg#:
    xor   dl,dl
    rol   byte [reg],4
    test  al,al
    check_zf swap_#reg
    write_flags
    ret
}

_swap a
_swap b
_swap c
_swap d
_swap e
_swap h
_swap l

swap__hl:
xor   eax,eax
push  eax
mov   ax,[hl]
push  eax
call  mem_readb
xor   dl,dl
ror   al,4
check_zf swap__hl
write_flags
mov   [esp+4],al
call  mem_writeb
add   esp,8
ret
