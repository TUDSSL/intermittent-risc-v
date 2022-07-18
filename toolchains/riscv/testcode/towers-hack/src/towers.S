
towers.elf:     file format elf64-littleriscv


Disassembly of section .text.init:

0000000080000000 <_start>:
    80000000:	4081                	li	ra,0
    80000002:	4101                	li	sp,0
    80000004:	4181                	li	gp,0
    80000006:	4201                	li	tp,0
    80000008:	4281                	li	t0,0
    8000000a:	4301                	li	t1,0
    8000000c:	4381                	li	t2,0
    8000000e:	4401                	li	s0,0
    80000010:	4481                	li	s1,0
    80000012:	4501                	li	a0,0
    80000014:	4581                	li	a1,0
    80000016:	4601                	li	a2,0
    80000018:	4681                	li	a3,0
    8000001a:	4701                	li	a4,0
    8000001c:	4781                	li	a5,0
    8000001e:	4801                	li	a6,0
    80000020:	4881                	li	a7,0
    80000022:	4901                	li	s2,0
    80000024:	4981                	li	s3,0
    80000026:	4a01                	li	s4,0
    80000028:	4a81                	li	s5,0
    8000002a:	4b01                	li	s6,0
    8000002c:	4b81                	li	s7,0
    8000002e:	4c01                	li	s8,0
    80000030:	4c81                	li	s9,0
    80000032:	4d01                	li	s10,0
    80000034:	4d81                	li	s11,0
    80000036:	4e01                	li	t3,0
    80000038:	4e81                	li	t4,0
    8000003a:	4f01                	li	t5,0
    8000003c:	4f81                	li	t6,0
    8000003e:	62f9                	lui	t0,0x1e
    80000040:	3002a073          	csrs	mstatus,t0
    80000044:	4285                	li	t0,1
    80000046:	02fe                	slli	t0,t0,0x1f
    80000048:	0002d863          	bgez	t0,80000058 <_start+0x58>
    8000004c:	4505                	li	a0,1
    8000004e:	00001297          	auipc	t0,0x1
    80000052:	faa2a923          	sw	a0,-78(t0) # 80001000 <tohost>
    80000056:	bfdd                	j	8000004c <_start+0x4c>
    80000058:	00000297          	auipc	t0,0x0
    8000005c:	03828293          	addi	t0,t0,56 # 80000090 <trap_entry>
    80000060:	30529073          	csrw	mtvec,t0
    80000064:	00004197          	auipc	gp,0x4
    80000068:	ec018193          	addi	gp,gp,-320 # 80003f24 <__global_pointer$>
    8000006c:	91b18213          	addi	tp,gp,-1765 # 8000383f <_end+0x3f>
    80000070:	fc027213          	andi	tp,tp,-64
    80000074:	f1402573          	csrr	a0,mhartid
    80000078:	4585                	li	a1,1
    8000007a:	00b57063          	bgeu	a0,a1,8000007a <_start+0x7a>
    8000007e:	00150113          	addi	sp,a0,1
    80000082:	0146                	slli	sp,sp,0x11
    80000084:	9112                	add	sp,sp,tp
    80000086:	01151613          	slli	a2,a0,0x11
    8000008a:	9232                	add	tp,tp,a2
    8000008c:	2900206f          	j	8000231c <_init>

0000000080000090 <trap_entry>:
    80000090:	716d                	addi	sp,sp,-272
    80000092:	e406                	sd	ra,8(sp)
    80000094:	e80a                	sd	sp,16(sp)
    80000096:	ec0e                	sd	gp,24(sp)
    80000098:	f012                	sd	tp,32(sp)
    8000009a:	f416                	sd	t0,40(sp)
    8000009c:	f81a                	sd	t1,48(sp)
    8000009e:	fc1e                	sd	t2,56(sp)
    800000a0:	e0a2                	sd	s0,64(sp)
    800000a2:	e4a6                	sd	s1,72(sp)
    800000a4:	e8aa                	sd	a0,80(sp)
    800000a6:	ecae                	sd	a1,88(sp)
    800000a8:	f0b2                	sd	a2,96(sp)
    800000aa:	f4b6                	sd	a3,104(sp)
    800000ac:	f8ba                	sd	a4,112(sp)
    800000ae:	fcbe                	sd	a5,120(sp)
    800000b0:	e142                	sd	a6,128(sp)
    800000b2:	e546                	sd	a7,136(sp)
    800000b4:	e94a                	sd	s2,144(sp)
    800000b6:	ed4e                	sd	s3,152(sp)
    800000b8:	f152                	sd	s4,160(sp)
    800000ba:	f556                	sd	s5,168(sp)
    800000bc:	f95a                	sd	s6,176(sp)
    800000be:	fd5e                	sd	s7,184(sp)
    800000c0:	e1e2                	sd	s8,192(sp)
    800000c2:	e5e6                	sd	s9,200(sp)
    800000c4:	e9ea                	sd	s10,208(sp)
    800000c6:	edee                	sd	s11,216(sp)
    800000c8:	f1f2                	sd	t3,224(sp)
    800000ca:	f5f6                	sd	t4,232(sp)
    800000cc:	f9fa                	sd	t5,240(sp)
    800000ce:	fdfe                	sd	t6,248(sp)
    800000d0:	34202573          	csrr	a0,mcause
    800000d4:	341025f3          	csrr	a1,mepc
    800000d8:	860a                	mv	a2,sp
    800000da:	0a6020ef          	jal	ra,80002180 <handle_trap>
    800000de:	34151073          	csrw	mepc,a0
    800000e2:	6289                	lui	t0,0x2
    800000e4:	8002829b          	addiw	t0,t0,-2048
    800000e8:	3002a073          	csrs	mstatus,t0
    800000ec:	60a2                	ld	ra,8(sp)
    800000ee:	6142                	ld	sp,16(sp)
    800000f0:	61e2                	ld	gp,24(sp)
    800000f2:	7202                	ld	tp,32(sp)
    800000f4:	72a2                	ld	t0,40(sp)
    800000f6:	7342                	ld	t1,48(sp)
    800000f8:	73e2                	ld	t2,56(sp)
    800000fa:	6406                	ld	s0,64(sp)
    800000fc:	64a6                	ld	s1,72(sp)
    800000fe:	6546                	ld	a0,80(sp)
    80000100:	65e6                	ld	a1,88(sp)
    80000102:	7606                	ld	a2,96(sp)
    80000104:	76a6                	ld	a3,104(sp)
    80000106:	7746                	ld	a4,112(sp)
    80000108:	77e6                	ld	a5,120(sp)
    8000010a:	680a                	ld	a6,128(sp)
    8000010c:	68aa                	ld	a7,136(sp)
    8000010e:	694a                	ld	s2,144(sp)
    80000110:	69ea                	ld	s3,152(sp)
    80000112:	7a0a                	ld	s4,160(sp)
    80000114:	7aaa                	ld	s5,168(sp)
    80000116:	7b4a                	ld	s6,176(sp)
    80000118:	7bea                	ld	s7,184(sp)
    8000011a:	6c0e                	ld	s8,192(sp)
    8000011c:	6cae                	ld	s9,200(sp)
    8000011e:	6d4e                	ld	s10,208(sp)
    80000120:	6dee                	ld	s11,216(sp)
    80000122:	7e0e                	ld	t3,224(sp)
    80000124:	7eae                	ld	t4,232(sp)
    80000126:	7f4e                	ld	t5,240(sp)
    80000128:	7fee                	ld	t6,248(sp)
    8000012a:	6151                	addi	sp,sp,272
    8000012c:	30200073          	mret

Disassembly of section .tohost:

0000000080001000 <tohost>:
	...

0000000080001040 <fromhost>:
	...

Disassembly of section .text:

0000000080002000 <setStats>:
    80002000:	715d                	addi	sp,sp,-80
    80002002:	e486                	sd	ra,72(sp)
    80002004:	e0a2                	sd	s0,64(sp)
    80002006:	0880                	addi	s0,sp,80
    80002008:	85aa                	mv	a1,a0
    8000200a:	fea42623          	sw	a0,-20(s0)
    8000200e:	4501                	li	a0,0
    80002010:	fea42423          	sw	a0,-24(s0)
    80002014:	0040006f          	j	80002018 <setStats+0x18>
    80002018:	0040006f          	j	8000201c <setStats+0x1c>
    8000201c:	fe842503          	lw	a0,-24(s0)
    80002020:	4589                	li	a1,2
    80002022:	00b54663          	blt	a0,a1,8000202e <setStats+0x2e>
    80002026:	0040006f          	j	8000202a <setStats+0x2a>
    8000202a:	ff3ff06f          	j	8000201c <setStats+0x1c>
    8000202e:	b0002573          	csrr	a0,mcycle
    80002032:	fca43c23          	sd	a0,-40(s0)
    80002036:	fd843503          	ld	a0,-40(s0)
    8000203a:	fca43823          	sd	a0,-48(s0)
    8000203e:	fd043503          	ld	a0,-48(s0)
    80002042:	fea43023          	sd	a0,-32(s0)
    80002046:	fec42503          	lw	a0,-20(s0)
    8000204a:	4581                	li	a1,0
    8000204c:	04b51263          	bne	a0,a1,80002090 <setStats+0x90>
    80002050:	0040006f          	j	80002054 <setStats+0x54>
    80002054:	fe842503          	lw	a0,-24(s0)
    80002058:	050e                	slli	a0,a0,0x3
    8000205a:	00001597          	auipc	a1,0x1
    8000205e:	6ce58593          	addi	a1,a1,1742 # 80003728 <counters>
    80002062:	952e                	add	a0,a0,a1
    80002064:	6108                	ld	a0,0(a0)
    80002066:	fe043583          	ld	a1,-32(s0)
    8000206a:	40a58533          	sub	a0,a1,a0
    8000206e:	fea43023          	sd	a0,-32(s0)
    80002072:	fe842503          	lw	a0,-24(s0)
    80002076:	050e                	slli	a0,a0,0x3
    80002078:	00001597          	auipc	a1,0x1
    8000207c:	6c058593          	addi	a1,a1,1728 # 80003738 <counter_names>
    80002080:	952e                	add	a0,a0,a1
    80002082:	00001597          	auipc	a1,0x1
    80002086:	66a58593          	addi	a1,a1,1642 # 800036ec <main+0xd6>
    8000208a:	e10c                	sd	a1,0(a0)
    8000208c:	0040006f          	j	80002090 <setStats+0x90>
    80002090:	fe043503          	ld	a0,-32(s0)
    80002094:	fe842583          	lw	a1,-24(s0)
    80002098:	00158613          	addi	a2,a1,1
    8000209c:	fec42423          	sw	a2,-24(s0)
    800020a0:	058e                	slli	a1,a1,0x3
    800020a2:	00001617          	auipc	a2,0x1
    800020a6:	68660613          	addi	a2,a2,1670 # 80003728 <counters>
    800020aa:	95b2                	add	a1,a1,a2
    800020ac:	e188                	sd	a0,0(a1)
    800020ae:	0040006f          	j	800020b2 <setStats+0xb2>
    800020b2:	0040006f          	j	800020b6 <setStats+0xb6>
    800020b6:	0040006f          	j	800020ba <setStats+0xba>
    800020ba:	fe842503          	lw	a0,-24(s0)
    800020be:	4589                	li	a1,2
    800020c0:	00b54663          	blt	a0,a1,800020cc <setStats+0xcc>
    800020c4:	0040006f          	j	800020c8 <setStats+0xc8>
    800020c8:	ff3ff06f          	j	800020ba <setStats+0xba>
    800020cc:	b0202573          	csrr	a0,minstret
    800020d0:	fca43023          	sd	a0,-64(s0)
    800020d4:	fc043503          	ld	a0,-64(s0)
    800020d8:	faa43c23          	sd	a0,-72(s0)
    800020dc:	fb843503          	ld	a0,-72(s0)
    800020e0:	fca43423          	sd	a0,-56(s0)
    800020e4:	fec42503          	lw	a0,-20(s0)
    800020e8:	4581                	li	a1,0
    800020ea:	04b51263          	bne	a0,a1,8000212e <setStats+0x12e>
    800020ee:	0040006f          	j	800020f2 <setStats+0xf2>
    800020f2:	fe842503          	lw	a0,-24(s0)
    800020f6:	050e                	slli	a0,a0,0x3
    800020f8:	00001597          	auipc	a1,0x1
    800020fc:	63058593          	addi	a1,a1,1584 # 80003728 <counters>
    80002100:	952e                	add	a0,a0,a1
    80002102:	6108                	ld	a0,0(a0)
    80002104:	fc843583          	ld	a1,-56(s0)
    80002108:	40a58533          	sub	a0,a1,a0
    8000210c:	fca43423          	sd	a0,-56(s0)
    80002110:	fe842503          	lw	a0,-24(s0)
    80002114:	050e                	slli	a0,a0,0x3
    80002116:	00001597          	auipc	a1,0x1
    8000211a:	62258593          	addi	a1,a1,1570 # 80003738 <counter_names>
    8000211e:	952e                	add	a0,a0,a1
    80002120:	00001597          	auipc	a1,0x1
    80002124:	5d358593          	addi	a1,a1,1491 # 800036f3 <main+0xdd>
    80002128:	e10c                	sd	a1,0(a0)
    8000212a:	0040006f          	j	8000212e <setStats+0x12e>
    8000212e:	fc843503          	ld	a0,-56(s0)
    80002132:	fe842583          	lw	a1,-24(s0)
    80002136:	00158613          	addi	a2,a1,1
    8000213a:	fec42423          	sw	a2,-24(s0)
    8000213e:	058e                	slli	a1,a1,0x3
    80002140:	00001617          	auipc	a2,0x1
    80002144:	5e860613          	addi	a2,a2,1512 # 80003728 <counters>
    80002148:	95b2                	add	a1,a1,a2
    8000214a:	e188                	sd	a0,0(a1)
    8000214c:	0040006f          	j	80002150 <setStats+0x150>
    80002150:	6406                	ld	s0,64(sp)
    80002152:	60a6                	ld	ra,72(sp)
    80002154:	6161                	addi	sp,sp,80
    80002156:	8082                	ret

0000000080002158 <tohost_exit>:
    80002158:	1101                	addi	sp,sp,-32
    8000215a:	ec06                	sd	ra,24(sp)
    8000215c:	e822                	sd	s0,16(sp)
    8000215e:	1000                	addi	s0,sp,32
    80002160:	fea43423          	sd	a0,-24(s0)
    80002164:	fe843503          	ld	a0,-24(s0)
    80002168:	0506                	slli	a0,a0,0x1
    8000216a:	00156513          	ori	a0,a0,1
    8000216e:	fffff597          	auipc	a1,0xfffff
    80002172:	e9258593          	addi	a1,a1,-366 # 80001000 <tohost>
    80002176:	e188                	sd	a0,0(a1)
    80002178:	0040006f          	j	8000217c <tohost_exit+0x24>
    8000217c:	0000006f          	j	8000217c <tohost_exit+0x24>

0000000080002180 <handle_trap>:
    80002180:	7179                	addi	sp,sp,-48
    80002182:	f406                	sd	ra,40(sp)
    80002184:	f022                	sd	s0,32(sp)
    80002186:	1800                	addi	s0,sp,48
    80002188:	fea43423          	sd	a0,-24(s0)
    8000218c:	feb43023          	sd	a1,-32(s0)
    80002190:	fcc43c23          	sd	a2,-40(s0)
    80002194:	53900513          	li	a0,1337
    80002198:	fc1ff0ef          	jal	ra,80002158 <tohost_exit>

000000008000219c <exit>:
    8000219c:	1101                	addi	sp,sp,-32
    8000219e:	ec06                	sd	ra,24(sp)
    800021a0:	e822                	sd	s0,16(sp)
    800021a2:	1000                	addi	s0,sp,32
    800021a4:	85aa                	mv	a1,a0
    800021a6:	fea42623          	sw	a0,-20(s0)
    800021aa:	fec42503          	lw	a0,-20(s0)
    800021ae:	feb43023          	sd	a1,-32(s0)
    800021b2:	fa7ff0ef          	jal	ra,80002158 <tohost_exit>

00000000800021b6 <abort>:
    800021b6:	1141                	addi	sp,sp,-16
    800021b8:	e406                	sd	ra,8(sp)
    800021ba:	e022                	sd	s0,0(sp)
    800021bc:	0800                	addi	s0,sp,16
    800021be:	08600513          	li	a0,134
    800021c2:	fdbff0ef          	jal	ra,8000219c <exit>

00000000800021c6 <printstr>:
    800021c6:	7179                	addi	sp,sp,-48
    800021c8:	f406                	sd	ra,40(sp)
    800021ca:	f022                	sd	s0,32(sp)
    800021cc:	1800                	addi	s0,sp,48
    800021ce:	fea43423          	sd	a0,-24(s0)
    800021d2:	fe843503          	ld	a0,-24(s0)
    800021d6:	fea43023          	sd	a0,-32(s0)
    800021da:	094000ef          	jal	ra,8000226e <strlen>
    800021de:	04000593          	li	a1,64
    800021e2:	4605                	li	a2,1
    800021e4:	fca43c23          	sd	a0,-40(s0)
    800021e8:	852e                	mv	a0,a1
    800021ea:	85b2                	mv	a1,a2
    800021ec:	fe043603          	ld	a2,-32(s0)
    800021f0:	fd843683          	ld	a3,-40(s0)
    800021f4:	00c000ef          	jal	ra,80002200 <syscall>
    800021f8:	7402                	ld	s0,32(sp)
    800021fa:	70a2                	ld	ra,40(sp)
    800021fc:	6145                	addi	sp,sp,48
    800021fe:	8082                	ret

0000000080002200 <syscall>:
    80002200:	7131                	addi	sp,sp,-192
    80002202:	fd06                	sd	ra,184(sp)
    80002204:	f922                	sd	s0,176(sp)
    80002206:	0180                	addi	s0,sp,192
    80002208:	fc017113          	andi	sp,sp,-64
    8000220c:	f52a                	sd	a0,168(sp)
    8000220e:	f12e                	sd	a1,160(sp)
    80002210:	ed32                	sd	a2,152(sp)
    80002212:	e936                	sd	a3,144(sp)
    80002214:	752a                	ld	a0,168(sp)
    80002216:	e0aa                	sd	a0,64(sp)
    80002218:	750a                	ld	a0,160(sp)
    8000221a:	e4aa                	sd	a0,72(sp)
    8000221c:	656a                	ld	a0,152(sp)
    8000221e:	e8aa                	sd	a0,80(sp)
    80002220:	654a                	ld	a0,144(sp)
    80002222:	ecaa                	sd	a0,88(sp)
    80002224:	0330000f          	fence	rw,rw
    80002228:	fffff517          	auipc	a0,0xfffff
    8000222c:	dd850513          	addi	a0,a0,-552 # 80001000 <tohost>
    80002230:	008c                	addi	a1,sp,64
    80002232:	e10c                	sd	a1,0(a0)
    80002234:	0040006f          	j	80002238 <syscall+0x38>
    80002238:	fffff517          	auipc	a0,0xfffff
    8000223c:	e0850513          	addi	a0,a0,-504 # 80001040 <fromhost>
    80002240:	6108                	ld	a0,0(a0)
    80002242:	4581                	li	a1,0
    80002244:	00b51663          	bne	a0,a1,80002250 <syscall+0x50>
    80002248:	0040006f          	j	8000224c <syscall+0x4c>
    8000224c:	fedff06f          	j	80002238 <syscall+0x38>
    80002250:	fffff517          	auipc	a0,0xfffff
    80002254:	df050513          	addi	a0,a0,-528 # 80001040 <fromhost>
    80002258:	4581                	li	a1,0
    8000225a:	e10c                	sd	a1,0(a0)
    8000225c:	0330000f          	fence	rw,rw
    80002260:	6506                	ld	a0,64(sp)
    80002262:	f4040113          	addi	sp,s0,-192
    80002266:	744a                	ld	s0,176(sp)
    80002268:	70ea                	ld	ra,184(sp)
    8000226a:	6129                	addi	sp,sp,192
    8000226c:	8082                	ret

000000008000226e <strlen>:
    8000226e:	1101                	addi	sp,sp,-32
    80002270:	ec06                	sd	ra,24(sp)
    80002272:	e822                	sd	s0,16(sp)
    80002274:	1000                	addi	s0,sp,32
    80002276:	fea43423          	sd	a0,-24(s0)
    8000227a:	fe843503          	ld	a0,-24(s0)
    8000227e:	fea43023          	sd	a0,-32(s0)
    80002282:	0040006f          	j	80002286 <strlen+0x18>
    80002286:	fe043503          	ld	a0,-32(s0)
    8000228a:	00054503          	lbu	a0,0(a0)
    8000228e:	4581                	li	a1,0
    80002290:	00b50b63          	beq	a0,a1,800022a6 <strlen+0x38>
    80002294:	0040006f          	j	80002298 <strlen+0x2a>
    80002298:	fe043503          	ld	a0,-32(s0)
    8000229c:	0505                	addi	a0,a0,1
    8000229e:	fea43023          	sd	a0,-32(s0)
    800022a2:	fe5ff06f          	j	80002286 <strlen+0x18>
    800022a6:	fe043503          	ld	a0,-32(s0)
    800022aa:	fe843583          	ld	a1,-24(s0)
    800022ae:	8d0d                	sub	a0,a0,a1
    800022b0:	6442                	ld	s0,16(sp)
    800022b2:	60e2                	ld	ra,24(sp)
    800022b4:	6105                	addi	sp,sp,32
    800022b6:	8082                	ret

00000000800022b8 <thread_entry>:
    800022b8:	1101                	addi	sp,sp,-32
    800022ba:	ec06                	sd	ra,24(sp)
    800022bc:	e822                	sd	s0,16(sp)
    800022be:	1000                	addi	s0,sp,32
    800022c0:	862e                	mv	a2,a1
    800022c2:	86aa                	mv	a3,a0
    800022c4:	fea42623          	sw	a0,-20(s0)
    800022c8:	feb42423          	sw	a1,-24(s0)
    800022cc:	0040006f          	j	800022d0 <thread_entry+0x18>
    800022d0:	fec42503          	lw	a0,-20(s0)
    800022d4:	4581                	li	a1,0
    800022d6:	00b50663          	beq	a0,a1,800022e2 <thread_entry+0x2a>
    800022da:	0040006f          	j	800022de <thread_entry+0x26>
    800022de:	ff3ff06f          	j	800022d0 <thread_entry+0x18>
    800022e2:	6442                	ld	s0,16(sp)
    800022e4:	60e2                	ld	ra,24(sp)
    800022e6:	6105                	addi	sp,sp,32
    800022e8:	8082                	ret
    800022ea:	7179                	addi	sp,sp,-48
    800022ec:	f406                	sd	ra,40(sp)
    800022ee:	f022                	sd	s0,32(sp)
    800022f0:	1800                	addi	s0,sp,48
    800022f2:	862a                	mv	a2,a0
    800022f4:	4681                	li	a3,0
    800022f6:	fed42623          	sw	a3,-20(s0)
    800022fa:	fea42423          	sw	a0,-24(s0)
    800022fe:	feb43023          	sd	a1,-32(s0)
    80002302:	00001517          	auipc	a0,0x1
    80002306:	3fa50513          	addi	a0,a0,1018 # 800036fc <main+0xe6>
    8000230a:	fcc43c23          	sd	a2,-40(s0)
    8000230e:	eb9ff0ef          	jal	ra,800021c6 <printstr>
    80002312:	557d                	li	a0,-1
    80002314:	7402                	ld	s0,32(sp)
    80002316:	70a2                	ld	ra,40(sp)
    80002318:	6145                	addi	sp,sp,48
    8000231a:	8082                	ret

000000008000231c <_init>:
    8000231c:	7111                	addi	sp,sp,-256
    8000231e:	fd86                	sd	ra,248(sp)
    80002320:	f9a2                	sd	s0,240(sp)
    80002322:	0200                	addi	s0,sp,256
    80002324:	fc017113          	andi	sp,sp,-64
    80002328:	862e                	mv	a2,a1
    8000232a:	86aa                	mv	a3,a0
    8000232c:	d7aa                	sw	a0,236(sp)
    8000232e:	d5ae                	sw	a1,232(sp)
    80002330:	f4b2                	sd	a2,104(sp)
    80002332:	f0b6                	sd	a3,96(sp)
    80002334:	489000ef          	jal	ra,80002fbc <init_tls>
    80002338:	553e                	lw	a0,236(sp)
    8000233a:	55ae                	lw	a1,232(sp)
    8000233c:	f7dff0ef          	jal	ra,800022b8 <thread_entry>
    80002340:	4501                	li	a0,0
    80002342:	ecaa                	sd	a0,88(sp)
    80002344:	65e6                	ld	a1,88(sp)
    80002346:	2d0010ef          	jal	ra,80003616 <main>
    8000234a:	d3aa                	sw	a0,228(sp)
    8000234c:	0108                	addi	a0,sp,128
    8000234e:	fcaa                	sd	a0,120(sp)
    80002350:	6566                	ld	a0,88(sp)
    80002352:	daaa                	sw	a0,116(sp)
    80002354:	0040006f          	j	80002358 <_init+0x3c>
    80002358:	5556                	lw	a0,116(sp)
    8000235a:	4585                	li	a1,1
    8000235c:	06a5c263          	blt	a1,a0,800023c0 <_init+0xa4>
    80002360:	0040006f          	j	80002364 <_init+0x48>
    80002364:	5556                	lw	a0,116(sp)
    80002366:	050e                	slli	a0,a0,0x3
    80002368:	00001597          	auipc	a1,0x1
    8000236c:	3c058593          	addi	a1,a1,960 # 80003728 <counters>
    80002370:	952e                	add	a0,a0,a1
    80002372:	6108                	ld	a0,0(a0)
    80002374:	4581                	li	a1,0
    80002376:	02b50e63          	beq	a0,a1,800023b2 <_init+0x96>
    8000237a:	0040006f          	j	8000237e <_init+0x62>
    8000237e:	7566                	ld	a0,120(sp)
    80002380:	55d6                	lw	a1,116(sp)
    80002382:	058e                	slli	a1,a1,0x3
    80002384:	00001617          	auipc	a2,0x1
    80002388:	3b460613          	addi	a2,a2,948 # 80003738 <counter_names>
    8000238c:	962e                	add	a2,a2,a1
    8000238e:	6210                	ld	a2,0(a2)
    80002390:	00001697          	auipc	a3,0x1
    80002394:	39868693          	addi	a3,a3,920 # 80003728 <counters>
    80002398:	95b6                	add	a1,a1,a3
    8000239a:	4194                	lw	a3,0(a1)
    8000239c:	00001597          	auipc	a1,0x1
    800023a0:	37858593          	addi	a1,a1,888 # 80003714 <main+0xfe>
    800023a4:	038000ef          	jal	ra,800023dc <sprintf>
    800023a8:	75e6                	ld	a1,120(sp)
    800023aa:	952e                	add	a0,a0,a1
    800023ac:	fcaa                	sd	a0,120(sp)
    800023ae:	0040006f          	j	800023b2 <_init+0x96>
    800023b2:	0040006f          	j	800023b6 <_init+0x9a>
    800023b6:	5556                	lw	a0,116(sp)
    800023b8:	0505                	addi	a0,a0,1
    800023ba:	daaa                	sw	a0,116(sp)
    800023bc:	f9dff06f          	j	80002358 <_init+0x3c>
    800023c0:	7566                	ld	a0,120(sp)
    800023c2:	010c                	addi	a1,sp,128
    800023c4:	00b50963          	beq	a0,a1,800023d6 <_init+0xba>
    800023c8:	0040006f          	j	800023cc <_init+0xb0>
    800023cc:	0108                	addi	a0,sp,128
    800023ce:	df9ff0ef          	jal	ra,800021c6 <printstr>
    800023d2:	0040006f          	j	800023d6 <_init+0xba>
    800023d6:	551e                	lw	a0,228(sp)
    800023d8:	dc5ff0ef          	jal	ra,8000219c <exit>

00000000800023dc <sprintf>:
    800023dc:	711d                	addi	sp,sp,-96
    800023de:	f406                	sd	ra,40(sp)
    800023e0:	f022                	sd	s0,32(sp)
    800023e2:	1800                	addi	s0,sp,48
    800023e4:	03143423          	sd	a7,40(s0)
    800023e8:	03043023          	sd	a6,32(s0)
    800023ec:	ec1c                	sd	a5,24(s0)
    800023ee:	e818                	sd	a4,16(s0)
    800023f0:	e414                	sd	a3,8(s0)
    800023f2:	e010                	sd	a2,0(s0)
    800023f4:	fea43423          	sd	a0,-24(s0)
    800023f8:	feb43023          	sd	a1,-32(s0)
    800023fc:	fe843503          	ld	a0,-24(s0)
    80002400:	fca43823          	sd	a0,-48(s0)
    80002404:	8522                	mv	a0,s0
    80002406:	fca43c23          	sd	a0,-40(s0)
    8000240a:	fe043603          	ld	a2,-32(s0)
    8000240e:	fd843683          	ld	a3,-40(s0)
    80002412:	00000517          	auipc	a0,0x0
    80002416:	75050513          	addi	a0,a0,1872 # 80002b62 <sprintf_putch>
    8000241a:	fe840593          	addi	a1,s0,-24
    8000241e:	180000ef          	jal	ra,8000259e <vprintfmt>
    80002422:	fe843503          	ld	a0,-24(s0)
    80002426:	4581                	li	a1,0
    80002428:	00b50023          	sb	a1,0(a0)
    8000242c:	fe843503          	ld	a0,-24(s0)
    80002430:	fd043583          	ld	a1,-48(s0)
    80002434:	9d0d                	subw	a0,a0,a1
    80002436:	7402                	ld	s0,32(sp)
    80002438:	70a2                	ld	ra,40(sp)
    8000243a:	6125                	addi	sp,sp,96
    8000243c:	8082                	ret

000000008000243e <putchar>:
    8000243e:	1101                	addi	sp,sp,-32
    80002440:	ec06                	sd	ra,24(sp)
    80002442:	e822                	sd	s0,16(sp)
    80002444:	1000                	addi	s0,sp,32
    80002446:	85aa                	mv	a1,a0
    80002448:	fea42623          	sw	a0,-20(s0)
    8000244c:	fec40503          	lb	a0,-20(s0)
    80002450:	04022683          	lw	a3,64(tp) # 40 <putchar.buflen>
    80002454:	00168713          	addi	a4,a3,1
    80002458:	04e22023          	sw	a4,64(tp) # 40 <putchar.buflen>
    8000245c:	00020613          	mv	a2,tp
    80002460:	9636                	add	a2,a2,a3
    80002462:	00a60023          	sb	a0,0(a2)
    80002466:	fec42503          	lw	a0,-20(s0)
    8000246a:	4629                	li	a2,10
    8000246c:	00c50c63          	beq	a0,a2,80002484 <putchar+0x46>
    80002470:	0040006f          	j	80002474 <putchar+0x36>
    80002474:	04022503          	lw	a0,64(tp) # 40 <putchar.buflen>
    80002478:	04000593          	li	a1,64
    8000247c:	02b51863          	bne	a0,a1,800024ac <putchar+0x6e>
    80002480:	0040006f          	j	80002484 <putchar+0x46>
    80002484:	04022683          	lw	a3,64(tp) # 40 <putchar.buflen>
    80002488:	00020613          	mv	a2,tp
    8000248c:	04000593          	li	a1,64
    80002490:	4705                	li	a4,1
    80002492:	fea43023          	sd	a0,-32(s0)
    80002496:	852e                	mv	a0,a1
    80002498:	85ba                	mv	a1,a4
    8000249a:	d67ff0ef          	jal	ra,80002200 <syscall>
    8000249e:	4581                	li	a1,0
    800024a0:	fe043603          	ld	a2,-32(s0)
    800024a4:	04b22023          	sw	a1,64(tp) # 40 <putchar.buflen>
    800024a8:	0040006f          	j	800024ac <putchar+0x6e>
    800024ac:	4501                	li	a0,0
    800024ae:	6442                	ld	s0,16(sp)
    800024b0:	60e2                	ld	ra,24(sp)
    800024b2:	6105                	addi	sp,sp,32
    800024b4:	8082                	ret

00000000800024b6 <printhex>:
    800024b6:	715d                	addi	sp,sp,-80
    800024b8:	e486                	sd	ra,72(sp)
    800024ba:	e0a2                	sd	s0,64(sp)
    800024bc:	0880                	addi	s0,sp,80
    800024be:	fea43423          	sd	a0,-24(s0)
    800024c2:	4501                	li	a0,0
    800024c4:	fca42823          	sw	a0,-48(s0)
    800024c8:	0040006f          	j	800024cc <printhex+0x16>
    800024cc:	fd042503          	lw	a0,-48(s0)
    800024d0:	45bd                	li	a1,15
    800024d2:	06a5c563          	blt	a1,a0,8000253c <printhex+0x86>
    800024d6:	0040006f          	j	800024da <printhex+0x24>
    800024da:	fe843503          	ld	a0,-24(s0)
    800024de:	893d                	andi	a0,a0,15
    800024e0:	05700593          	li	a1,87
    800024e4:	03000613          	li	a2,48
    800024e8:	46a9                	li	a3,10
    800024ea:	fca43423          	sd	a0,-56(s0)
    800024ee:	fcb43023          	sd	a1,-64(s0)
    800024f2:	fac43c23          	sd	a2,-72(s0)
    800024f6:	00d56663          	bltu	a0,a3,80002502 <printhex+0x4c>
    800024fa:	fc043503          	ld	a0,-64(s0)
    800024fe:	faa43c23          	sd	a0,-72(s0)
    80002502:	fb843503          	ld	a0,-72(s0)
    80002506:	fc843583          	ld	a1,-56(s0)
    8000250a:	952e                	add	a0,a0,a1
    8000250c:	fd042603          	lw	a2,-48(s0)
    80002510:	46bd                	li	a3,15
    80002512:	40c6863b          	subw	a2,a3,a2
    80002516:	fd740693          	addi	a3,s0,-41
    8000251a:	9636                	add	a2,a2,a3
    8000251c:	00a60023          	sb	a0,0(a2)
    80002520:	fe843503          	ld	a0,-24(s0)
    80002524:	8111                	srli	a0,a0,0x4
    80002526:	fea43423          	sd	a0,-24(s0)
    8000252a:	0040006f          	j	8000252e <printhex+0x78>
    8000252e:	fd042503          	lw	a0,-48(s0)
    80002532:	0505                	addi	a0,a0,1
    80002534:	fca42823          	sw	a0,-48(s0)
    80002538:	f95ff06f          	j	800024cc <printhex+0x16>
    8000253c:	4501                	li	a0,0
    8000253e:	fea403a3          	sb	a0,-25(s0)
    80002542:	fd740513          	addi	a0,s0,-41
    80002546:	c81ff0ef          	jal	ra,800021c6 <printstr>
    8000254a:	6406                	ld	s0,64(sp)
    8000254c:	60a6                	ld	ra,72(sp)
    8000254e:	6161                	addi	sp,sp,80
    80002550:	8082                	ret

0000000080002552 <printf>:
    80002552:	7159                	addi	sp,sp,-112
    80002554:	f406                	sd	ra,40(sp)
    80002556:	f022                	sd	s0,32(sp)
    80002558:	1800                	addi	s0,sp,48
    8000255a:	03143c23          	sd	a7,56(s0)
    8000255e:	03043823          	sd	a6,48(s0)
    80002562:	f41c                	sd	a5,40(s0)
    80002564:	f018                	sd	a4,32(s0)
    80002566:	ec14                	sd	a3,24(s0)
    80002568:	e810                	sd	a2,16(s0)
    8000256a:	e40c                	sd	a1,8(s0)
    8000256c:	fea43423          	sd	a0,-24(s0)
    80002570:	00840513          	addi	a0,s0,8
    80002574:	fea43023          	sd	a0,-32(s0)
    80002578:	fe843603          	ld	a2,-24(s0)
    8000257c:	fe043683          	ld	a3,-32(s0)
    80002580:	00000517          	auipc	a0,0x0
    80002584:	ebe50513          	addi	a0,a0,-322 # 8000243e <putchar>
    80002588:	4581                	li	a1,0
    8000258a:	fcb43c23          	sd	a1,-40(s0)
    8000258e:	010000ef          	jal	ra,8000259e <vprintfmt>
    80002592:	fd843503          	ld	a0,-40(s0)
    80002596:	7402                	ld	s0,32(sp)
    80002598:	70a2                	ld	ra,40(sp)
    8000259a:	6165                	addi	sp,sp,112
    8000259c:	8082                	ret

000000008000259e <vprintfmt>:
    8000259e:	7155                	addi	sp,sp,-208
    800025a0:	e586                	sd	ra,200(sp)
    800025a2:	e1a2                	sd	s0,192(sp)
    800025a4:	0980                	addi	s0,sp,208
    800025a6:	fea43423          	sd	a0,-24(s0)
    800025aa:	feb43023          	sd	a1,-32(s0)
    800025ae:	fcc43c23          	sd	a2,-40(s0)
    800025b2:	fcd43823          	sd	a3,-48(s0)
    800025b6:	0040006f          	j	800025ba <vprintfmt+0x1c>
    800025ba:	0040006f          	j	800025be <vprintfmt+0x20>
    800025be:	fd843503          	ld	a0,-40(s0)
    800025c2:	00054503          	lbu	a0,0(a0)
    800025c6:	faa42e23          	sw	a0,-68(s0)
    800025ca:	02500593          	li	a1,37
    800025ce:	04b50363          	beq	a0,a1,80002614 <vprintfmt+0x76>
    800025d2:	0040006f          	j	800025d6 <vprintfmt+0x38>
    800025d6:	fbc42503          	lw	a0,-68(s0)
    800025da:	4581                	li	a1,0
    800025dc:	00b51863          	bne	a0,a1,800025ec <vprintfmt+0x4e>
    800025e0:	0040006f          	j	800025e4 <vprintfmt+0x46>
    800025e4:	640e                	ld	s0,192(sp)
    800025e6:	60ae                	ld	ra,200(sp)
    800025e8:	6169                	addi	sp,sp,208
    800025ea:	8082                	ret
    800025ec:	fd843503          	ld	a0,-40(s0)
    800025f0:	0505                	addi	a0,a0,1
    800025f2:	fca43c23          	sd	a0,-40(s0)
    800025f6:	fe843503          	ld	a0,-24(s0)
    800025fa:	fbc42583          	lw	a1,-68(s0)
    800025fe:	fe043603          	ld	a2,-32(s0)
    80002602:	f8a43823          	sd	a0,-112(s0)
    80002606:	852e                	mv	a0,a1
    80002608:	85b2                	mv	a1,a2
    8000260a:	f9043603          	ld	a2,-112(s0)
    8000260e:	9602                	jalr	a2
    80002610:	fafff06f          	j	800025be <vprintfmt+0x20>
    80002614:	fd843503          	ld	a0,-40(s0)
    80002618:	0505                	addi	a0,a0,1
    8000261a:	fca43c23          	sd	a0,-40(s0)
    8000261e:	fd843503          	ld	a0,-40(s0)
    80002622:	fca43023          	sd	a0,-64(s0)
    80002626:	02000513          	li	a0,32
    8000262a:	f8a40da3          	sb	a0,-101(s0)
    8000262e:	4505                	li	a0,1
    80002630:	1502                	slli	a0,a0,0x20
    80002632:	157d                	addi	a0,a0,-1
    80002634:	faa42223          	sw	a0,-92(s0)
    80002638:	faa42023          	sw	a0,-96(s0)
    8000263c:	4501                	li	a0,0
    8000263e:	faa42423          	sw	a0,-88(s0)
    80002642:	f8a42e23          	sw	a0,-100(s0)
    80002646:	0040006f          	j	8000264a <vprintfmt+0xac>
    8000264a:	fd843503          	ld	a0,-40(s0)
    8000264e:	00150593          	addi	a1,a0,1
    80002652:	fcb43c23          	sd	a1,-40(s0)
    80002656:	00054503          	lbu	a0,0(a0)
    8000265a:	faa42e23          	sw	a0,-68(s0)
    8000265e:	02300593          	li	a1,35
    80002662:	f8a43423          	sd	a0,-120(s0)
    80002666:	1eb50563          	beq	a0,a1,80002850 <vprintfmt+0x2b2>
    8000266a:	0040006f          	j	8000266e <vprintfmt+0xd0>
    8000266e:	f8843503          	ld	a0,-120(s0)
    80002672:	02051593          	slli	a1,a0,0x20
    80002676:	9181                	srli	a1,a1,0x20
    80002678:	02500613          	li	a2,37
    8000267c:	4ac58063          	beq	a1,a2,80002b1c <vprintfmt+0x57e>
    80002680:	0040006f          	j	80002684 <vprintfmt+0xe6>
    80002684:	f8843503          	ld	a0,-120(s0)
    80002688:	02051593          	slli	a1,a0,0x20
    8000268c:	9181                	srli	a1,a1,0x20
    8000268e:	02a00613          	li	a2,42
    80002692:	18c58663          	beq	a1,a2,8000281e <vprintfmt+0x280>
    80002696:	0040006f          	j	8000269a <vprintfmt+0xfc>
    8000269a:	f8843503          	ld	a0,-120(s0)
    8000269e:	02051593          	slli	a1,a0,0x20
    800026a2:	9181                	srli	a1,a1,0x20
    800026a4:	02d00613          	li	a2,45
    800026a8:	0ec58b63          	beq	a1,a2,8000279e <vprintfmt+0x200>
    800026ac:	0040006f          	j	800026b0 <vprintfmt+0x112>
    800026b0:	f8843503          	ld	a0,-120(s0)
    800026b4:	02051593          	slli	a1,a0,0x20
    800026b8:	9181                	srli	a1,a1,0x20
    800026ba:	02e00613          	li	a2,46
    800026be:	16c58b63          	beq	a1,a2,80002834 <vprintfmt+0x296>
    800026c2:	0040006f          	j	800026c6 <vprintfmt+0x128>
    800026c6:	f8843503          	ld	a0,-120(s0)
    800026ca:	02051593          	slli	a1,a0,0x20
    800026ce:	9181                	srli	a1,a1,0x20
    800026d0:	03000613          	li	a2,48
    800026d4:	0cc58b63          	beq	a1,a2,800027aa <vprintfmt+0x20c>
    800026d8:	0040006f          	j	800026dc <vprintfmt+0x13e>
    800026dc:	f8843503          	ld	a0,-120(s0)
    800026e0:	fcf5059b          	addiw	a1,a0,-49
    800026e4:	4625                	li	a2,9
    800026e6:	0cc5e863          	bltu	a1,a2,800027b6 <vprintfmt+0x218>
    800026ea:	0040006f          	j	800026ee <vprintfmt+0x150>
    800026ee:	f8843503          	ld	a0,-120(s0)
    800026f2:	02051593          	slli	a1,a0,0x20
    800026f6:	9181                	srli	a1,a1,0x20
    800026f8:	06300613          	li	a2,99
    800026fc:	18c58a63          	beq	a1,a2,80002890 <vprintfmt+0x2f2>
    80002700:	0040006f          	j	80002704 <vprintfmt+0x166>
    80002704:	f8843503          	ld	a0,-120(s0)
    80002708:	02051593          	slli	a1,a0,0x20
    8000270c:	9181                	srli	a1,a1,0x20
    8000270e:	06400613          	li	a2,100
    80002712:	32c58663          	beq	a1,a2,80002a3e <vprintfmt+0x4a0>
    80002716:	0040006f          	j	8000271a <vprintfmt+0x17c>
    8000271a:	f8843503          	ld	a0,-120(s0)
    8000271e:	02051593          	slli	a1,a0,0x20
    80002722:	9181                	srli	a1,a1,0x20
    80002724:	06c00613          	li	a2,108
    80002728:	14c58d63          	beq	a1,a2,80002882 <vprintfmt+0x2e4>
    8000272c:	0040006f          	j	80002730 <vprintfmt+0x192>
    80002730:	f8843503          	ld	a0,-120(s0)
    80002734:	02051593          	slli	a1,a0,0x20
    80002738:	9181                	srli	a1,a1,0x20
    8000273a:	06f00613          	li	a2,111
    8000273e:	34c58e63          	beq	a1,a2,80002a9a <vprintfmt+0x4fc>
    80002742:	0040006f          	j	80002746 <vprintfmt+0x1a8>
    80002746:	f8843503          	ld	a0,-120(s0)
    8000274a:	02051593          	slli	a1,a0,0x20
    8000274e:	9181                	srli	a1,a1,0x20
    80002750:	07000613          	li	a2,112
    80002754:	34c58863          	beq	a1,a2,80002aa4 <vprintfmt+0x506>
    80002758:	0040006f          	j	8000275c <vprintfmt+0x1be>
    8000275c:	f8843503          	ld	a0,-120(s0)
    80002760:	02051593          	slli	a1,a0,0x20
    80002764:	9181                	srli	a1,a1,0x20
    80002766:	07300613          	li	a2,115
    8000276a:	14c58763          	beq	a1,a2,800028b8 <vprintfmt+0x31a>
    8000276e:	0040006f          	j	80002772 <vprintfmt+0x1d4>
    80002772:	f8843503          	ld	a0,-120(s0)
    80002776:	02051593          	slli	a1,a0,0x20
    8000277a:	9181                	srli	a1,a1,0x20
    8000277c:	07500613          	li	a2,117
    80002780:	30c58863          	beq	a1,a2,80002a90 <vprintfmt+0x4f2>
    80002784:	0040006f          	j	80002788 <vprintfmt+0x1ea>
    80002788:	f8843503          	ld	a0,-120(s0)
    8000278c:	02051593          	slli	a1,a0,0x20
    80002790:	9181                	srli	a1,a1,0x20
    80002792:	07800613          	li	a2,120
    80002796:	34c58463          	beq	a1,a2,80002ade <vprintfmt+0x540>
    8000279a:	3a00006f          	j	80002b3a <vprintfmt+0x59c>
    8000279e:	02d00513          	li	a0,45
    800027a2:	f8a40da3          	sb	a0,-101(s0)
    800027a6:	ea5ff06f          	j	8000264a <vprintfmt+0xac>
    800027aa:	03000513          	li	a0,48
    800027ae:	f8a40da3          	sb	a0,-101(s0)
    800027b2:	e99ff06f          	j	8000264a <vprintfmt+0xac>
    800027b6:	4501                	li	a0,0
    800027b8:	faa42023          	sw	a0,-96(s0)
    800027bc:	0040006f          	j	800027c0 <vprintfmt+0x222>
    800027c0:	fa042503          	lw	a0,-96(s0)
    800027c4:	45a9                	li	a1,10
    800027c6:	02b50533          	mul	a0,a0,a1
    800027ca:	fbc42583          	lw	a1,-68(s0)
    800027ce:	952e                	add	a0,a0,a1
    800027d0:	fd050513          	addi	a0,a0,-48
    800027d4:	faa42023          	sw	a0,-96(s0)
    800027d8:	fd843503          	ld	a0,-40(s0)
    800027dc:	00054503          	lbu	a0,0(a0)
    800027e0:	faa42e23          	sw	a0,-68(s0)
    800027e4:	fbc42503          	lw	a0,-68(s0)
    800027e8:	03000593          	li	a1,48
    800027ec:	00b54c63          	blt	a0,a1,80002804 <vprintfmt+0x266>
    800027f0:	0040006f          	j	800027f4 <vprintfmt+0x256>
    800027f4:	fbc42503          	lw	a0,-68(s0)
    800027f8:	03a00593          	li	a1,58
    800027fc:	00b54663          	blt	a0,a1,80002808 <vprintfmt+0x26a>
    80002800:	0040006f          	j	80002804 <vprintfmt+0x266>
    80002804:	0160006f          	j	8000281a <vprintfmt+0x27c>
    80002808:	0040006f          	j	8000280c <vprintfmt+0x26e>
    8000280c:	fd843503          	ld	a0,-40(s0)
    80002810:	0505                	addi	a0,a0,1
    80002812:	fca43c23          	sd	a0,-40(s0)
    80002816:	fabff06f          	j	800027c0 <vprintfmt+0x222>
    8000281a:	0400006f          	j	8000285a <vprintfmt+0x2bc>
    8000281e:	fd043503          	ld	a0,-48(s0)
    80002822:	00850593          	addi	a1,a0,8
    80002826:	fcb43823          	sd	a1,-48(s0)
    8000282a:	4108                	lw	a0,0(a0)
    8000282c:	faa42023          	sw	a0,-96(s0)
    80002830:	02a0006f          	j	8000285a <vprintfmt+0x2bc>
    80002834:	fa442503          	lw	a0,-92(s0)
    80002838:	55fd                	li	a1,-1
    8000283a:	00a5c963          	blt	a1,a0,8000284c <vprintfmt+0x2ae>
    8000283e:	0040006f          	j	80002842 <vprintfmt+0x2a4>
    80002842:	4501                	li	a0,0
    80002844:	faa42223          	sw	a0,-92(s0)
    80002848:	0040006f          	j	8000284c <vprintfmt+0x2ae>
    8000284c:	dffff06f          	j	8000264a <vprintfmt+0xac>
    80002850:	4505                	li	a0,1
    80002852:	f8a42e23          	sw	a0,-100(s0)
    80002856:	df5ff06f          	j	8000264a <vprintfmt+0xac>
    8000285a:	fa442503          	lw	a0,-92(s0)
    8000285e:	55fd                	li	a1,-1
    80002860:	00a5cf63          	blt	a1,a0,8000287e <vprintfmt+0x2e0>
    80002864:	0040006f          	j	80002868 <vprintfmt+0x2ca>
    80002868:	fa042503          	lw	a0,-96(s0)
    8000286c:	faa42223          	sw	a0,-92(s0)
    80002870:	4505                	li	a0,1
    80002872:	1502                	slli	a0,a0,0x20
    80002874:	157d                	addi	a0,a0,-1
    80002876:	faa42023          	sw	a0,-96(s0)
    8000287a:	0040006f          	j	8000287e <vprintfmt+0x2e0>
    8000287e:	dcdff06f          	j	8000264a <vprintfmt+0xac>
    80002882:	fa842503          	lw	a0,-88(s0)
    80002886:	0505                	addi	a0,a0,1
    80002888:	faa42423          	sw	a0,-88(s0)
    8000288c:	dbfff06f          	j	8000264a <vprintfmt+0xac>
    80002890:	fe843503          	ld	a0,-24(s0)
    80002894:	fd043583          	ld	a1,-48(s0)
    80002898:	00858613          	addi	a2,a1,8
    8000289c:	fcc43823          	sd	a2,-48(s0)
    800028a0:	418c                	lw	a1,0(a1)
    800028a2:	fe043603          	ld	a2,-32(s0)
    800028a6:	f8a43023          	sd	a0,-128(s0)
    800028aa:	852e                	mv	a0,a1
    800028ac:	85b2                	mv	a1,a2
    800028ae:	f8043603          	ld	a2,-128(s0)
    800028b2:	9602                	jalr	a2
    800028b4:	2aa0006f          	j	80002b5e <vprintfmt+0x5c0>
    800028b8:	fd043503          	ld	a0,-48(s0)
    800028bc:	00850593          	addi	a1,a0,8
    800028c0:	fcb43823          	sd	a1,-48(s0)
    800028c4:	6108                	ld	a0,0(a0)
    800028c6:	fca43423          	sd	a0,-56(s0)
    800028ca:	4581                	li	a1,0
    800028cc:	00b51c63          	bne	a0,a1,800028e4 <vprintfmt+0x346>
    800028d0:	0040006f          	j	800028d4 <vprintfmt+0x336>
    800028d4:	00001517          	auipc	a0,0x1
    800028d8:	e4950513          	addi	a0,a0,-439 # 8000371d <main+0x107>
    800028dc:	fca43423          	sd	a0,-56(s0)
    800028e0:	0040006f          	j	800028e4 <vprintfmt+0x346>
    800028e4:	fa442503          	lw	a0,-92(s0)
    800028e8:	4585                	li	a1,1
    800028ea:	06b54963          	blt	a0,a1,8000295c <vprintfmt+0x3be>
    800028ee:	0040006f          	j	800028f2 <vprintfmt+0x354>
    800028f2:	f9b44503          	lbu	a0,-101(s0)
    800028f6:	02d00593          	li	a1,45
    800028fa:	06b50163          	beq	a0,a1,8000295c <vprintfmt+0x3be>
    800028fe:	0040006f          	j	80002902 <vprintfmt+0x364>
    80002902:	fc843503          	ld	a0,-56(s0)
    80002906:	fa042583          	lw	a1,-96(s0)
    8000290a:	452000ef          	jal	ra,80002d5c <strnlen>
    8000290e:	fa442583          	lw	a1,-92(s0)
    80002912:	40a58533          	sub	a0,a1,a0
    80002916:	faa42223          	sw	a0,-92(s0)
    8000291a:	0040006f          	j	8000291e <vprintfmt+0x380>
    8000291e:	fa442503          	lw	a0,-92(s0)
    80002922:	4585                	li	a1,1
    80002924:	02b54a63          	blt	a0,a1,80002958 <vprintfmt+0x3ba>
    80002928:	0040006f          	j	8000292c <vprintfmt+0x38e>
    8000292c:	fe843503          	ld	a0,-24(s0)
    80002930:	f9b44583          	lbu	a1,-101(s0)
    80002934:	fe043603          	ld	a2,-32(s0)
    80002938:	f6a43c23          	sd	a0,-136(s0)
    8000293c:	852e                	mv	a0,a1
    8000293e:	85b2                	mv	a1,a2
    80002940:	f7843603          	ld	a2,-136(s0)
    80002944:	9602                	jalr	a2
    80002946:	0040006f          	j	8000294a <vprintfmt+0x3ac>
    8000294a:	fa442503          	lw	a0,-92(s0)
    8000294e:	157d                	addi	a0,a0,-1
    80002950:	faa42223          	sw	a0,-92(s0)
    80002954:	fcbff06f          	j	8000291e <vprintfmt+0x380>
    80002958:	0040006f          	j	8000295c <vprintfmt+0x3be>
    8000295c:	0040006f          	j	80002960 <vprintfmt+0x3c2>
    80002960:	fc843503          	ld	a0,-56(s0)
    80002964:	00054503          	lbu	a0,0(a0)
    80002968:	faa42e23          	sw	a0,-68(s0)
    8000296c:	4581                	li	a1,0
    8000296e:	862e                	mv	a2,a1
    80002970:	f6c43823          	sd	a2,-144(s0)
    80002974:	04b50263          	beq	a0,a1,800029b8 <vprintfmt+0x41a>
    80002978:	0040006f          	j	8000297c <vprintfmt+0x3de>
    8000297c:	fa042503          	lw	a0,-96(s0)
    80002980:	4585                	li	a1,1
    80002982:	4601                	li	a2,0
    80002984:	f6b43423          	sd	a1,-152(s0)
    80002988:	02c54263          	blt	a0,a2,800029ac <vprintfmt+0x40e>
    8000298c:	0040006f          	j	80002990 <vprintfmt+0x3f2>
    80002990:	fa042503          	lw	a0,-96(s0)
    80002994:	fff5059b          	addiw	a1,a0,-1
    80002998:	157d                	addi	a0,a0,-1
    8000299a:	faa42023          	sw	a0,-96(s0)
    8000299e:	557d                	li	a0,-1
    800029a0:	00b52533          	slt	a0,a0,a1
    800029a4:	f6a43423          	sd	a0,-152(s0)
    800029a8:	0040006f          	j	800029ac <vprintfmt+0x40e>
    800029ac:	f6843503          	ld	a0,-152(s0)
    800029b0:	f6a43823          	sd	a0,-144(s0)
    800029b4:	0040006f          	j	800029b8 <vprintfmt+0x41a>
    800029b8:	f7043503          	ld	a0,-144(s0)
    800029bc:	8905                	andi	a0,a0,1
    800029be:	4581                	li	a1,0
    800029c0:	02b50f63          	beq	a0,a1,800029fe <vprintfmt+0x460>
    800029c4:	0040006f          	j	800029c8 <vprintfmt+0x42a>
    800029c8:	fe843503          	ld	a0,-24(s0)
    800029cc:	fbc42583          	lw	a1,-68(s0)
    800029d0:	fe043603          	ld	a2,-32(s0)
    800029d4:	f6a43023          	sd	a0,-160(s0)
    800029d8:	852e                	mv	a0,a1
    800029da:	85b2                	mv	a1,a2
    800029dc:	f6043603          	ld	a2,-160(s0)
    800029e0:	9602                	jalr	a2
    800029e2:	fc843503          	ld	a0,-56(s0)
    800029e6:	0505                	addi	a0,a0,1
    800029e8:	fca43423          	sd	a0,-56(s0)
    800029ec:	0040006f          	j	800029f0 <vprintfmt+0x452>
    800029f0:	fa442503          	lw	a0,-92(s0)
    800029f4:	157d                	addi	a0,a0,-1
    800029f6:	faa42223          	sw	a0,-92(s0)
    800029fa:	f67ff06f          	j	80002960 <vprintfmt+0x3c2>
    800029fe:	0040006f          	j	80002a02 <vprintfmt+0x464>
    80002a02:	fa442503          	lw	a0,-92(s0)
    80002a06:	4585                	li	a1,1
    80002a08:	02b54963          	blt	a0,a1,80002a3a <vprintfmt+0x49c>
    80002a0c:	0040006f          	j	80002a10 <vprintfmt+0x472>
    80002a10:	fe843503          	ld	a0,-24(s0)
    80002a14:	fe043583          	ld	a1,-32(s0)
    80002a18:	02000613          	li	a2,32
    80002a1c:	f4a43c23          	sd	a0,-168(s0)
    80002a20:	8532                	mv	a0,a2
    80002a22:	f5843603          	ld	a2,-168(s0)
    80002a26:	9602                	jalr	a2
    80002a28:	0040006f          	j	80002a2c <vprintfmt+0x48e>
    80002a2c:	fa442503          	lw	a0,-92(s0)
    80002a30:	157d                	addi	a0,a0,-1
    80002a32:	faa42223          	sw	a0,-92(s0)
    80002a36:	fcdff06f          	j	80002a02 <vprintfmt+0x464>
    80002a3a:	1240006f          	j	80002b5e <vprintfmt+0x5c0>
    80002a3e:	fa842583          	lw	a1,-88(s0)
    80002a42:	fd040513          	addi	a0,s0,-48
    80002a46:	5de000ef          	jal	ra,80003024 <getint>
    80002a4a:	faa43823          	sd	a0,-80(s0)
    80002a4e:	fb043503          	ld	a0,-80(s0)
    80002a52:	55fd                	li	a1,-1
    80002a54:	02a5c963          	blt	a1,a0,80002a86 <vprintfmt+0x4e8>
    80002a58:	0040006f          	j	80002a5c <vprintfmt+0x4be>
    80002a5c:	fe843503          	ld	a0,-24(s0)
    80002a60:	fe043583          	ld	a1,-32(s0)
    80002a64:	02d00613          	li	a2,45
    80002a68:	f4a43823          	sd	a0,-176(s0)
    80002a6c:	8532                	mv	a0,a2
    80002a6e:	f5043603          	ld	a2,-176(s0)
    80002a72:	9602                	jalr	a2
    80002a74:	fb043503          	ld	a0,-80(s0)
    80002a78:	4581                	li	a1,0
    80002a7a:	40a58533          	sub	a0,a1,a0
    80002a7e:	faa43823          	sd	a0,-80(s0)
    80002a82:	0040006f          	j	80002a86 <vprintfmt+0x4e8>
    80002a86:	4529                	li	a0,10
    80002a88:	faa42623          	sw	a0,-84(s0)
    80002a8c:	0700006f          	j	80002afc <vprintfmt+0x55e>
    80002a90:	4529                	li	a0,10
    80002a92:	faa42623          	sw	a0,-84(s0)
    80002a96:	0520006f          	j	80002ae8 <vprintfmt+0x54a>
    80002a9a:	4521                	li	a0,8
    80002a9c:	faa42623          	sw	a0,-84(s0)
    80002aa0:	0480006f          	j	80002ae8 <vprintfmt+0x54a>
    80002aa4:	4505                	li	a0,1
    80002aa6:	faa42423          	sw	a0,-88(s0)
    80002aaa:	fe843503          	ld	a0,-24(s0)
    80002aae:	fe043583          	ld	a1,-32(s0)
    80002ab2:	03000613          	li	a2,48
    80002ab6:	f4a43423          	sd	a0,-184(s0)
    80002aba:	8532                	mv	a0,a2
    80002abc:	f4843603          	ld	a2,-184(s0)
    80002ac0:	9602                	jalr	a2
    80002ac2:	fe843503          	ld	a0,-24(s0)
    80002ac6:	fe043583          	ld	a1,-32(s0)
    80002aca:	07800613          	li	a2,120
    80002ace:	f4a43023          	sd	a0,-192(s0)
    80002ad2:	8532                	mv	a0,a2
    80002ad4:	f4043603          	ld	a2,-192(s0)
    80002ad8:	9602                	jalr	a2
    80002ada:	0040006f          	j	80002ade <vprintfmt+0x540>
    80002ade:	4541                	li	a0,16
    80002ae0:	faa42623          	sw	a0,-84(s0)
    80002ae4:	0040006f          	j	80002ae8 <vprintfmt+0x54a>
    80002ae8:	fa842583          	lw	a1,-88(s0)
    80002aec:	fd040513          	addi	a0,s0,-48
    80002af0:	5b0000ef          	jal	ra,800030a0 <getuint>
    80002af4:	faa43823          	sd	a0,-80(s0)
    80002af8:	0040006f          	j	80002afc <vprintfmt+0x55e>
    80002afc:	fe843503          	ld	a0,-24(s0)
    80002b00:	fe043583          	ld	a1,-32(s0)
    80002b04:	fb043603          	ld	a2,-80(s0)
    80002b08:	fac42683          	lw	a3,-84(s0)
    80002b0c:	fa442703          	lw	a4,-92(s0)
    80002b10:	f9b44783          	lbu	a5,-101(s0)
    80002b14:	60a000ef          	jal	ra,8000311e <printnum>
    80002b18:	0460006f          	j	80002b5e <vprintfmt+0x5c0>
    80002b1c:	fe843503          	ld	a0,-24(s0)
    80002b20:	fbc42583          	lw	a1,-68(s0)
    80002b24:	fe043603          	ld	a2,-32(s0)
    80002b28:	f2a43c23          	sd	a0,-200(s0)
    80002b2c:	852e                	mv	a0,a1
    80002b2e:	85b2                	mv	a1,a2
    80002b30:	f3843603          	ld	a2,-200(s0)
    80002b34:	9602                	jalr	a2
    80002b36:	0280006f          	j	80002b5e <vprintfmt+0x5c0>
    80002b3a:	fe843503          	ld	a0,-24(s0)
    80002b3e:	fe043583          	ld	a1,-32(s0)
    80002b42:	02500613          	li	a2,37
    80002b46:	f2a43823          	sd	a0,-208(s0)
    80002b4a:	8532                	mv	a0,a2
    80002b4c:	f3043603          	ld	a2,-208(s0)
    80002b50:	9602                	jalr	a2
    80002b52:	fc043503          	ld	a0,-64(s0)
    80002b56:	fca43c23          	sd	a0,-40(s0)
    80002b5a:	0040006f          	j	80002b5e <vprintfmt+0x5c0>
    80002b5e:	a5dff06f          	j	800025ba <vprintfmt+0x1c>

0000000080002b62 <sprintf_putch>:
    80002b62:	7179                	addi	sp,sp,-48
    80002b64:	f406                	sd	ra,40(sp)
    80002b66:	f022                	sd	s0,32(sp)
    80002b68:	1800                	addi	s0,sp,48
    80002b6a:	862a                	mv	a2,a0
    80002b6c:	fea42623          	sw	a0,-20(s0)
    80002b70:	feb43023          	sd	a1,-32(s0)
    80002b74:	fe043503          	ld	a0,-32(s0)
    80002b78:	fca43c23          	sd	a0,-40(s0)
    80002b7c:	fec40503          	lb	a0,-20(s0)
    80002b80:	fd843583          	ld	a1,-40(s0)
    80002b84:	618c                	ld	a1,0(a1)
    80002b86:	00a58023          	sb	a0,0(a1)
    80002b8a:	fd843503          	ld	a0,-40(s0)
    80002b8e:	610c                	ld	a1,0(a0)
    80002b90:	0585                	addi	a1,a1,1
    80002b92:	e10c                	sd	a1,0(a0)
    80002b94:	7402                	ld	s0,32(sp)
    80002b96:	70a2                	ld	ra,40(sp)
    80002b98:	6145                	addi	sp,sp,48
    80002b9a:	8082                	ret

0000000080002b9c <memcpy>:
    80002b9c:	715d                	addi	sp,sp,-80
    80002b9e:	e486                	sd	ra,72(sp)
    80002ba0:	e0a2                	sd	s0,64(sp)
    80002ba2:	0880                	addi	s0,sp,80
    80002ba4:	fea43423          	sd	a0,-24(s0)
    80002ba8:	feb43023          	sd	a1,-32(s0)
    80002bac:	fcc43c23          	sd	a2,-40(s0)
    80002bb0:	fe843503          	ld	a0,-24(s0)
    80002bb4:	fe043583          	ld	a1,-32(s0)
    80002bb8:	8d4d                	or	a0,a0,a1
    80002bba:	fd843583          	ld	a1,-40(s0)
    80002bbe:	8d4d                	or	a0,a0,a1
    80002bc0:	891d                	andi	a0,a0,7
    80002bc2:	4581                	li	a1,0
    80002bc4:	04b51b63          	bne	a0,a1,80002c1a <memcpy+0x7e>
    80002bc8:	0040006f          	j	80002bcc <memcpy+0x30>
    80002bcc:	fe043503          	ld	a0,-32(s0)
    80002bd0:	fca43823          	sd	a0,-48(s0)
    80002bd4:	fe843503          	ld	a0,-24(s0)
    80002bd8:	fca43423          	sd	a0,-56(s0)
    80002bdc:	0040006f          	j	80002be0 <memcpy+0x44>
    80002be0:	fc843503          	ld	a0,-56(s0)
    80002be4:	fe843583          	ld	a1,-24(s0)
    80002be8:	fd843603          	ld	a2,-40(s0)
    80002bec:	95b2                	add	a1,a1,a2
    80002bee:	02b57463          	bgeu	a0,a1,80002c16 <memcpy+0x7a>
    80002bf2:	0040006f          	j	80002bf6 <memcpy+0x5a>
    80002bf6:	fd043503          	ld	a0,-48(s0)
    80002bfa:	00850593          	addi	a1,a0,8
    80002bfe:	fcb43823          	sd	a1,-48(s0)
    80002c02:	6108                	ld	a0,0(a0)
    80002c04:	fc843583          	ld	a1,-56(s0)
    80002c08:	00858613          	addi	a2,a1,8
    80002c0c:	fcc43423          	sd	a2,-56(s0)
    80002c10:	e188                	sd	a0,0(a1)
    80002c12:	fcfff06f          	j	80002be0 <memcpy+0x44>
    80002c16:	0560006f          	j	80002c6c <memcpy+0xd0>
    80002c1a:	fe043503          	ld	a0,-32(s0)
    80002c1e:	fca43023          	sd	a0,-64(s0)
    80002c22:	fe843503          	ld	a0,-24(s0)
    80002c26:	faa43c23          	sd	a0,-72(s0)
    80002c2a:	0040006f          	j	80002c2e <memcpy+0x92>
    80002c2e:	fb843503          	ld	a0,-72(s0)
    80002c32:	fe843583          	ld	a1,-24(s0)
    80002c36:	fd843603          	ld	a2,-40(s0)
    80002c3a:	95b2                	add	a1,a1,a2
    80002c3c:	02b57663          	bgeu	a0,a1,80002c68 <memcpy+0xcc>
    80002c40:	0040006f          	j	80002c44 <memcpy+0xa8>
    80002c44:	fc043503          	ld	a0,-64(s0)
    80002c48:	00150593          	addi	a1,a0,1
    80002c4c:	fcb43023          	sd	a1,-64(s0)
    80002c50:	00050503          	lb	a0,0(a0)
    80002c54:	fb843583          	ld	a1,-72(s0)
    80002c58:	00158613          	addi	a2,a1,1
    80002c5c:	fac43c23          	sd	a2,-72(s0)
    80002c60:	00a58023          	sb	a0,0(a1)
    80002c64:	fcbff06f          	j	80002c2e <memcpy+0x92>
    80002c68:	0040006f          	j	80002c6c <memcpy+0xd0>
    80002c6c:	fe843503          	ld	a0,-24(s0)
    80002c70:	6406                	ld	s0,64(sp)
    80002c72:	60a6                	ld	ra,72(sp)
    80002c74:	6161                	addi	sp,sp,80
    80002c76:	8082                	ret

0000000080002c78 <memset>:
    80002c78:	7139                	addi	sp,sp,-64
    80002c7a:	fc06                	sd	ra,56(sp)
    80002c7c:	f822                	sd	s0,48(sp)
    80002c7e:	0080                	addi	s0,sp,64
    80002c80:	86ae                	mv	a3,a1
    80002c82:	fea43423          	sd	a0,-24(s0)
    80002c86:	feb42223          	sw	a1,-28(s0)
    80002c8a:	fcc43c23          	sd	a2,-40(s0)
    80002c8e:	fe843503          	ld	a0,-24(s0)
    80002c92:	fd843583          	ld	a1,-40(s0)
    80002c96:	8d4d                	or	a0,a0,a1
    80002c98:	891d                	andi	a0,a0,7
    80002c9a:	4581                	li	a1,0
    80002c9c:	06b51b63          	bne	a0,a1,80002d12 <memset+0x9a>
    80002ca0:	0040006f          	j	80002ca4 <memset+0x2c>
    80002ca4:	fe444503          	lbu	a0,-28(s0)
    80002ca8:	fca43823          	sd	a0,-48(s0)
    80002cac:	fd043503          	ld	a0,-48(s0)
    80002cb0:	00851593          	slli	a1,a0,0x8
    80002cb4:	8d4d                	or	a0,a0,a1
    80002cb6:	fca43823          	sd	a0,-48(s0)
    80002cba:	fd043503          	ld	a0,-48(s0)
    80002cbe:	01051593          	slli	a1,a0,0x10
    80002cc2:	8d4d                	or	a0,a0,a1
    80002cc4:	fca43823          	sd	a0,-48(s0)
    80002cc8:	fd043503          	ld	a0,-48(s0)
    80002ccc:	02051593          	slli	a1,a0,0x20
    80002cd0:	8d4d                	or	a0,a0,a1
    80002cd2:	fca43823          	sd	a0,-48(s0)
    80002cd6:	fe843503          	ld	a0,-24(s0)
    80002cda:	fca43423          	sd	a0,-56(s0)
    80002cde:	0040006f          	j	80002ce2 <memset+0x6a>
    80002ce2:	fc843503          	ld	a0,-56(s0)
    80002ce6:	fe843583          	ld	a1,-24(s0)
    80002cea:	fd843603          	ld	a2,-40(s0)
    80002cee:	95b2                	add	a1,a1,a2
    80002cf0:	00b57f63          	bgeu	a0,a1,80002d0e <memset+0x96>
    80002cf4:	0040006f          	j	80002cf8 <memset+0x80>
    80002cf8:	fd043503          	ld	a0,-48(s0)
    80002cfc:	fc843583          	ld	a1,-56(s0)
    80002d00:	00858613          	addi	a2,a1,8
    80002d04:	fcc43423          	sd	a2,-56(s0)
    80002d08:	e188                	sd	a0,0(a1)
    80002d0a:	fd9ff06f          	j	80002ce2 <memset+0x6a>
    80002d0e:	0420006f          	j	80002d50 <memset+0xd8>
    80002d12:	fe843503          	ld	a0,-24(s0)
    80002d16:	fca43023          	sd	a0,-64(s0)
    80002d1a:	0040006f          	j	80002d1e <memset+0xa6>
    80002d1e:	fc043503          	ld	a0,-64(s0)
    80002d22:	fe843583          	ld	a1,-24(s0)
    80002d26:	fd843603          	ld	a2,-40(s0)
    80002d2a:	95b2                	add	a1,a1,a2
    80002d2c:	02b57063          	bgeu	a0,a1,80002d4c <memset+0xd4>
    80002d30:	0040006f          	j	80002d34 <memset+0xbc>
    80002d34:	fe440503          	lb	a0,-28(s0)
    80002d38:	fc043583          	ld	a1,-64(s0)
    80002d3c:	00158613          	addi	a2,a1,1
    80002d40:	fcc43023          	sd	a2,-64(s0)
    80002d44:	00a58023          	sb	a0,0(a1)
    80002d48:	fd7ff06f          	j	80002d1e <memset+0xa6>
    80002d4c:	0040006f          	j	80002d50 <memset+0xd8>
    80002d50:	fe843503          	ld	a0,-24(s0)
    80002d54:	7442                	ld	s0,48(sp)
    80002d56:	70e2                	ld	ra,56(sp)
    80002d58:	6121                	addi	sp,sp,64
    80002d5a:	8082                	ret

0000000080002d5c <strnlen>:
    80002d5c:	7179                	addi	sp,sp,-48
    80002d5e:	f406                	sd	ra,40(sp)
    80002d60:	f022                	sd	s0,32(sp)
    80002d62:	1800                	addi	s0,sp,48
    80002d64:	fea43423          	sd	a0,-24(s0)
    80002d68:	feb43023          	sd	a1,-32(s0)
    80002d6c:	fe843503          	ld	a0,-24(s0)
    80002d70:	fca43c23          	sd	a0,-40(s0)
    80002d74:	0040006f          	j	80002d78 <strnlen+0x1c>
    80002d78:	fe043503          	ld	a0,-32(s0)
    80002d7c:	fff50593          	addi	a1,a0,-1
    80002d80:	feb43023          	sd	a1,-32(s0)
    80002d84:	4581                	li	a1,0
    80002d86:	862e                	mv	a2,a1
    80002d88:	fcc43823          	sd	a2,-48(s0)
    80002d8c:	00b50e63          	beq	a0,a1,80002da8 <strnlen+0x4c>
    80002d90:	0040006f          	j	80002d94 <strnlen+0x38>
    80002d94:	fd843503          	ld	a0,-40(s0)
    80002d98:	00054503          	lbu	a0,0(a0)
    80002d9c:	00a03533          	snez	a0,a0
    80002da0:	fca43823          	sd	a0,-48(s0)
    80002da4:	0040006f          	j	80002da8 <strnlen+0x4c>
    80002da8:	fd043503          	ld	a0,-48(s0)
    80002dac:	8905                	andi	a0,a0,1
    80002dae:	4581                	li	a1,0
    80002db0:	00b50b63          	beq	a0,a1,80002dc6 <strnlen+0x6a>
    80002db4:	0040006f          	j	80002db8 <strnlen+0x5c>
    80002db8:	fd843503          	ld	a0,-40(s0)
    80002dbc:	0505                	addi	a0,a0,1
    80002dbe:	fca43c23          	sd	a0,-40(s0)
    80002dc2:	fb7ff06f          	j	80002d78 <strnlen+0x1c>
    80002dc6:	fd843503          	ld	a0,-40(s0)
    80002dca:	fe843583          	ld	a1,-24(s0)
    80002dce:	8d0d                	sub	a0,a0,a1
    80002dd0:	7402                	ld	s0,32(sp)
    80002dd2:	70a2                	ld	ra,40(sp)
    80002dd4:	6145                	addi	sp,sp,48
    80002dd6:	8082                	ret

0000000080002dd8 <strcmp>:
    80002dd8:	7179                	addi	sp,sp,-48
    80002dda:	f406                	sd	ra,40(sp)
    80002ddc:	f022                	sd	s0,32(sp)
    80002dde:	1800                	addi	s0,sp,48
    80002de0:	fea43423          	sd	a0,-24(s0)
    80002de4:	feb43023          	sd	a1,-32(s0)
    80002de8:	0040006f          	j	80002dec <strcmp+0x14>
    80002dec:	fe843503          	ld	a0,-24(s0)
    80002df0:	00150593          	addi	a1,a0,1
    80002df4:	feb43423          	sd	a1,-24(s0)
    80002df8:	00050503          	lb	a0,0(a0)
    80002dfc:	fca40fa3          	sb	a0,-33(s0)
    80002e00:	fe043503          	ld	a0,-32(s0)
    80002e04:	00150593          	addi	a1,a0,1
    80002e08:	feb43023          	sd	a1,-32(s0)
    80002e0c:	00050503          	lb	a0,0(a0)
    80002e10:	fca40f23          	sb	a0,-34(s0)
    80002e14:	0040006f          	j	80002e18 <strcmp+0x40>
    80002e18:	fdf44503          	lbu	a0,-33(s0)
    80002e1c:	4581                	li	a1,0
    80002e1e:	862e                	mv	a2,a1
    80002e20:	fcc43823          	sd	a2,-48(s0)
    80002e24:	00b50f63          	beq	a0,a1,80002e42 <strcmp+0x6a>
    80002e28:	0040006f          	j	80002e2c <strcmp+0x54>
    80002e2c:	fdf44503          	lbu	a0,-33(s0)
    80002e30:	fde44583          	lbu	a1,-34(s0)
    80002e34:	8d2d                	xor	a0,a0,a1
    80002e36:	00153513          	seqz	a0,a0
    80002e3a:	fca43823          	sd	a0,-48(s0)
    80002e3e:	0040006f          	j	80002e42 <strcmp+0x6a>
    80002e42:	fd043503          	ld	a0,-48(s0)
    80002e46:	8905                	andi	a0,a0,1
    80002e48:	fa0512e3          	bnez	a0,80002dec <strcmp+0x14>
    80002e4c:	0040006f          	j	80002e50 <strcmp+0x78>
    80002e50:	fdf44503          	lbu	a0,-33(s0)
    80002e54:	fde44583          	lbu	a1,-34(s0)
    80002e58:	8d0d                	sub	a0,a0,a1
    80002e5a:	7402                	ld	s0,32(sp)
    80002e5c:	70a2                	ld	ra,40(sp)
    80002e5e:	6145                	addi	sp,sp,48
    80002e60:	8082                	ret

0000000080002e62 <strcpy>:
    80002e62:	7179                	addi	sp,sp,-48
    80002e64:	f406                	sd	ra,40(sp)
    80002e66:	f022                	sd	s0,32(sp)
    80002e68:	1800                	addi	s0,sp,48
    80002e6a:	fea43423          	sd	a0,-24(s0)
    80002e6e:	feb43023          	sd	a1,-32(s0)
    80002e72:	fe843503          	ld	a0,-24(s0)
    80002e76:	fca43c23          	sd	a0,-40(s0)
    80002e7a:	0040006f          	j	80002e7e <strcpy+0x1c>
    80002e7e:	fe043503          	ld	a0,-32(s0)
    80002e82:	00150593          	addi	a1,a0,1
    80002e86:	feb43023          	sd	a1,-32(s0)
    80002e8a:	00054503          	lbu	a0,0(a0)
    80002e8e:	fd843583          	ld	a1,-40(s0)
    80002e92:	00158613          	addi	a2,a1,1
    80002e96:	fcc43c23          	sd	a2,-40(s0)
    80002e9a:	00a58023          	sb	a0,0(a1)
    80002e9e:	4581                	li	a1,0
    80002ea0:	00b50663          	beq	a0,a1,80002eac <strcpy+0x4a>
    80002ea4:	0040006f          	j	80002ea8 <strcpy+0x46>
    80002ea8:	fd7ff06f          	j	80002e7e <strcpy+0x1c>
    80002eac:	fe843503          	ld	a0,-24(s0)
    80002eb0:	7402                	ld	s0,32(sp)
    80002eb2:	70a2                	ld	ra,40(sp)
    80002eb4:	6145                	addi	sp,sp,48
    80002eb6:	8082                	ret

0000000080002eb8 <atol>:
    80002eb8:	7179                	addi	sp,sp,-48
    80002eba:	f406                	sd	ra,40(sp)
    80002ebc:	f022                	sd	s0,32(sp)
    80002ebe:	1800                	addi	s0,sp,48
    80002ec0:	fea43423          	sd	a0,-24(s0)
    80002ec4:	4501                	li	a0,0
    80002ec6:	fea43023          	sd	a0,-32(s0)
    80002eca:	fca42e23          	sw	a0,-36(s0)
    80002ece:	0040006f          	j	80002ed2 <atol+0x1a>
    80002ed2:	fe843503          	ld	a0,-24(s0)
    80002ed6:	00054503          	lbu	a0,0(a0)
    80002eda:	02000593          	li	a1,32
    80002ede:	00b51b63          	bne	a0,a1,80002ef4 <atol+0x3c>
    80002ee2:	0040006f          	j	80002ee6 <atol+0x2e>
    80002ee6:	fe843503          	ld	a0,-24(s0)
    80002eea:	0505                	addi	a0,a0,1
    80002eec:	fea43423          	sd	a0,-24(s0)
    80002ef0:	fe3ff06f          	j	80002ed2 <atol+0x1a>
    80002ef4:	fe843503          	ld	a0,-24(s0)
    80002ef8:	00054503          	lbu	a0,0(a0)
    80002efc:	02d00593          	li	a1,45
    80002f00:	00b50e63          	beq	a0,a1,80002f1c <atol+0x64>
    80002f04:	0040006f          	j	80002f08 <atol+0x50>
    80002f08:	fe843503          	ld	a0,-24(s0)
    80002f0c:	00054503          	lbu	a0,0(a0)
    80002f10:	02b00593          	li	a1,43
    80002f14:	02b51563          	bne	a0,a1,80002f3e <atol+0x86>
    80002f18:	0040006f          	j	80002f1c <atol+0x64>
    80002f1c:	fe843503          	ld	a0,-24(s0)
    80002f20:	00054503          	lbu	a0,0(a0)
    80002f24:	02d54513          	xori	a0,a0,45
    80002f28:	00153513          	seqz	a0,a0
    80002f2c:	fca42e23          	sw	a0,-36(s0)
    80002f30:	fe843503          	ld	a0,-24(s0)
    80002f34:	0505                	addi	a0,a0,1
    80002f36:	fea43423          	sd	a0,-24(s0)
    80002f3a:	0040006f          	j	80002f3e <atol+0x86>
    80002f3e:	0040006f          	j	80002f42 <atol+0x8a>
    80002f42:	fe843503          	ld	a0,-24(s0)
    80002f46:	00054503          	lbu	a0,0(a0)
    80002f4a:	4581                	li	a1,0
    80002f4c:	02b50c63          	beq	a0,a1,80002f84 <atol+0xcc>
    80002f50:	0040006f          	j	80002f54 <atol+0x9c>
    80002f54:	fe043503          	ld	a0,-32(s0)
    80002f58:	45a9                	li	a1,10
    80002f5a:	02b50533          	mul	a0,a0,a1
    80002f5e:	fea43023          	sd	a0,-32(s0)
    80002f62:	fe843503          	ld	a0,-24(s0)
    80002f66:	00150593          	addi	a1,a0,1
    80002f6a:	feb43423          	sd	a1,-24(s0)
    80002f6e:	00054503          	lbu	a0,0(a0)
    80002f72:	fe043583          	ld	a1,-32(s0)
    80002f76:	952e                	add	a0,a0,a1
    80002f78:	fd050513          	addi	a0,a0,-48
    80002f7c:	fea43023          	sd	a0,-32(s0)
    80002f80:	fc3ff06f          	j	80002f42 <atol+0x8a>
    80002f84:	fdc42503          	lw	a0,-36(s0)
    80002f88:	4581                	li	a1,0
    80002f8a:	00b50d63          	beq	a0,a1,80002fa4 <atol+0xec>
    80002f8e:	0040006f          	j	80002f92 <atol+0xda>
    80002f92:	fe043503          	ld	a0,-32(s0)
    80002f96:	4581                	li	a1,0
    80002f98:	40a58533          	sub	a0,a1,a0
    80002f9c:	fca43823          	sd	a0,-48(s0)
    80002fa0:	0100006f          	j	80002fb0 <atol+0xf8>
    80002fa4:	fe043503          	ld	a0,-32(s0)
    80002fa8:	fca43823          	sd	a0,-48(s0)
    80002fac:	0040006f          	j	80002fb0 <atol+0xf8>
    80002fb0:	fd043503          	ld	a0,-48(s0)
    80002fb4:	7402                	ld	s0,32(sp)
    80002fb6:	70a2                	ld	ra,40(sp)
    80002fb8:	6145                	addi	sp,sp,48
    80002fba:	8082                	ret

0000000080002fbc <init_tls>:
    80002fbc:	7139                	addi	sp,sp,-64
    80002fbe:	fc06                	sd	ra,56(sp)
    80002fc0:	f822                	sd	s0,48(sp)
    80002fc2:	0080                	addi	s0,sp,64
    80002fc4:	8a418513          	addi	a0,gp,-1884 # 800037c8 <_tdata_begin>
    80002fc8:	8a418593          	addi	a1,gp,-1884 # 800037c8 <_tdata_begin>
    80002fcc:	40a58633          	sub	a2,a1,a0
    80002fd0:	fec43023          	sd	a2,-32(s0)
    80002fd4:	fe843603          	ld	a2,-24(s0)
    80002fd8:	fe043683          	ld	a3,-32(s0)
    80002fdc:	fca43823          	sd	a0,-48(s0)
    80002fe0:	8532                	mv	a0,a2
    80002fe2:	fd043603          	ld	a2,-48(s0)
    80002fe6:	fcb43423          	sd	a1,-56(s0)
    80002fea:	85b2                	mv	a1,a2
    80002fec:	8636                	mv	a2,a3
    80002fee:	bafff0ef          	jal	ra,80002b9c <memcpy>
    80002ff2:	92018593          	addi	a1,gp,-1760 # 80003844 <_tbss_end>
    80002ff6:	fc843603          	ld	a2,-56(s0)
    80002ffa:	8d91                	sub	a1,a1,a2
    80002ffc:	fcb43c23          	sd	a1,-40(s0)
    80003000:	fe843583          	ld	a1,-24(s0)
    80003004:	fe043683          	ld	a3,-32(s0)
    80003008:	95b6                	add	a1,a1,a3
    8000300a:	fd843603          	ld	a2,-40(s0)
    8000300e:	4681                	li	a3,0
    80003010:	fca43023          	sd	a0,-64(s0)
    80003014:	852e                	mv	a0,a1
    80003016:	85b6                	mv	a1,a3
    80003018:	c61ff0ef          	jal	ra,80002c78 <memset>
    8000301c:	7442                	ld	s0,48(sp)
    8000301e:	70e2                	ld	ra,56(sp)
    80003020:	6121                	addi	sp,sp,64
    80003022:	8082                	ret

0000000080003024 <getint>:
    80003024:	7179                	addi	sp,sp,-48
    80003026:	f406                	sd	ra,40(sp)
    80003028:	f022                	sd	s0,32(sp)
    8000302a:	1800                	addi	s0,sp,48
    8000302c:	862e                	mv	a2,a1
    8000302e:	fea43023          	sd	a0,-32(s0)
    80003032:	fcb42e23          	sw	a1,-36(s0)
    80003036:	fdc42503          	lw	a0,-36(s0)
    8000303a:	4589                	li	a1,2
    8000303c:	00b54f63          	blt	a0,a1,8000305a <getint+0x36>
    80003040:	0040006f          	j	80003044 <getint+0x20>
    80003044:	fe043503          	ld	a0,-32(s0)
    80003048:	610c                	ld	a1,0(a0)
    8000304a:	00858613          	addi	a2,a1,8
    8000304e:	e110                	sd	a2,0(a0)
    80003050:	6188                	ld	a0,0(a1)
    80003052:	fea43423          	sd	a0,-24(s0)
    80003056:	03e0006f          	j	80003094 <getint+0x70>
    8000305a:	fdc42503          	lw	a0,-36(s0)
    8000305e:	4581                	li	a1,0
    80003060:	00b50f63          	beq	a0,a1,8000307e <getint+0x5a>
    80003064:	0040006f          	j	80003068 <getint+0x44>
    80003068:	fe043503          	ld	a0,-32(s0)
    8000306c:	610c                	ld	a1,0(a0)
    8000306e:	00858613          	addi	a2,a1,8
    80003072:	e110                	sd	a2,0(a0)
    80003074:	6188                	ld	a0,0(a1)
    80003076:	fea43423          	sd	a0,-24(s0)
    8000307a:	01a0006f          	j	80003094 <getint+0x70>
    8000307e:	fe043503          	ld	a0,-32(s0)
    80003082:	610c                	ld	a1,0(a0)
    80003084:	00858613          	addi	a2,a1,8
    80003088:	e110                	sd	a2,0(a0)
    8000308a:	4188                	lw	a0,0(a1)
    8000308c:	fea43423          	sd	a0,-24(s0)
    80003090:	0040006f          	j	80003094 <getint+0x70>
    80003094:	fe843503          	ld	a0,-24(s0)
    80003098:	7402                	ld	s0,32(sp)
    8000309a:	70a2                	ld	ra,40(sp)
    8000309c:	6145                	addi	sp,sp,48
    8000309e:	8082                	ret

00000000800030a0 <getuint>:
    800030a0:	7179                	addi	sp,sp,-48
    800030a2:	f406                	sd	ra,40(sp)
    800030a4:	f022                	sd	s0,32(sp)
    800030a6:	1800                	addi	s0,sp,48
    800030a8:	862e                	mv	a2,a1
    800030aa:	fea43023          	sd	a0,-32(s0)
    800030ae:	fcb42e23          	sw	a1,-36(s0)
    800030b2:	fdc42503          	lw	a0,-36(s0)
    800030b6:	4589                	li	a1,2
    800030b8:	00b54f63          	blt	a0,a1,800030d6 <getuint+0x36>
    800030bc:	0040006f          	j	800030c0 <getuint+0x20>
    800030c0:	fe043503          	ld	a0,-32(s0)
    800030c4:	610c                	ld	a1,0(a0)
    800030c6:	00858613          	addi	a2,a1,8
    800030ca:	e110                	sd	a2,0(a0)
    800030cc:	6188                	ld	a0,0(a1)
    800030ce:	fea43423          	sd	a0,-24(s0)
    800030d2:	0400006f          	j	80003112 <getuint+0x72>
    800030d6:	fdc42503          	lw	a0,-36(s0)
    800030da:	4581                	li	a1,0
    800030dc:	00b50f63          	beq	a0,a1,800030fa <getuint+0x5a>
    800030e0:	0040006f          	j	800030e4 <getuint+0x44>
    800030e4:	fe043503          	ld	a0,-32(s0)
    800030e8:	610c                	ld	a1,0(a0)
    800030ea:	00858613          	addi	a2,a1,8
    800030ee:	e110                	sd	a2,0(a0)
    800030f0:	6188                	ld	a0,0(a1)
    800030f2:	fea43423          	sd	a0,-24(s0)
    800030f6:	01c0006f          	j	80003112 <getuint+0x72>
    800030fa:	fe043503          	ld	a0,-32(s0)
    800030fe:	610c                	ld	a1,0(a0)
    80003100:	00858613          	addi	a2,a1,8
    80003104:	e110                	sd	a2,0(a0)
    80003106:	0005e503          	lwu	a0,0(a1)
    8000310a:	fea43423          	sd	a0,-24(s0)
    8000310e:	0040006f          	j	80003112 <getuint+0x72>
    80003112:	fe843503          	ld	a0,-24(s0)
    80003116:	7402                	ld	s0,32(sp)
    80003118:	70a2                	ld	ra,40(sp)
    8000311a:	6145                	addi	sp,sp,48
    8000311c:	8082                	ret

000000008000311e <printnum>:
    8000311e:	710d                	addi	sp,sp,-352
    80003120:	ee86                	sd	ra,344(sp)
    80003122:	eaa2                	sd	s0,336(sp)
    80003124:	1280                	addi	s0,sp,352
    80003126:	883e                	mv	a6,a5
    80003128:	88ba                	mv	a7,a4
    8000312a:	82b6                	mv	t0,a3
    8000312c:	fea43423          	sd	a0,-24(s0)
    80003130:	feb43023          	sd	a1,-32(s0)
    80003134:	fcc43c23          	sd	a2,-40(s0)
    80003138:	fcd42a23          	sw	a3,-44(s0)
    8000313c:	fce42823          	sw	a4,-48(s0)
    80003140:	fcf42623          	sw	a5,-52(s0)
    80003144:	4501                	li	a0,0
    80003146:	eca42423          	sw	a0,-312(s0)
    8000314a:	0040006f          	j	8000314e <printnum+0x30>
    8000314e:	fd843503          	ld	a0,-40(s0)
    80003152:	fd446583          	lwu	a1,-44(s0)
    80003156:	02b57533          	remu	a0,a0,a1
    8000315a:	ec842583          	lw	a1,-312(s0)
    8000315e:	00158613          	addi	a2,a1,1
    80003162:	ecc42423          	sw	a2,-312(s0)
    80003166:	058a                	slli	a1,a1,0x2
    80003168:	ecc40613          	addi	a2,s0,-308
    8000316c:	95b2                	add	a1,a1,a2
    8000316e:	c188                	sw	a0,0(a1)
    80003170:	fd843503          	ld	a0,-40(s0)
    80003174:	fd446583          	lwu	a1,-44(s0)
    80003178:	00b57663          	bgeu	a0,a1,80003184 <printnum+0x66>
    8000317c:	0040006f          	j	80003180 <printnum+0x62>
    80003180:	0180006f          	j	80003198 <printnum+0x7a>
    80003184:	fd446503          	lwu	a0,-44(s0)
    80003188:	fd843583          	ld	a1,-40(s0)
    8000318c:	02a5d533          	divu	a0,a1,a0
    80003190:	fca43c23          	sd	a0,-40(s0)
    80003194:	fbbff06f          	j	8000314e <printnum+0x30>
    80003198:	0040006f          	j	8000319c <printnum+0x7e>
    8000319c:	fd042503          	lw	a0,-48(s0)
    800031a0:	fff50593          	addi	a1,a0,-1
    800031a4:	fcb42823          	sw	a1,-48(s0)
    800031a8:	ec842583          	lw	a1,-312(s0)
    800031ac:	02a5d363          	bge	a1,a0,800031d2 <printnum+0xb4>
    800031b0:	0040006f          	j	800031b4 <printnum+0x96>
    800031b4:	fe843503          	ld	a0,-24(s0)
    800031b8:	fcc42583          	lw	a1,-52(s0)
    800031bc:	fe043603          	ld	a2,-32(s0)
    800031c0:	eca43023          	sd	a0,-320(s0)
    800031c4:	852e                	mv	a0,a1
    800031c6:	85b2                	mv	a1,a2
    800031c8:	ec043603          	ld	a2,-320(s0)
    800031cc:	9602                	jalr	a2
    800031ce:	fcfff06f          	j	8000319c <printnum+0x7e>
    800031d2:	0040006f          	j	800031d6 <printnum+0xb8>
    800031d6:	ec842503          	lw	a0,-312(s0)
    800031da:	fff50593          	addi	a1,a0,-1
    800031de:	ecb42423          	sw	a1,-312(s0)
    800031e2:	4585                	li	a1,1
    800031e4:	04b54c63          	blt	a0,a1,8000323c <printnum+0x11e>
    800031e8:	0040006f          	j	800031ec <printnum+0xce>
    800031ec:	fe843503          	ld	a0,-24(s0)
    800031f0:	ec842583          	lw	a1,-312(s0)
    800031f4:	058a                	slli	a1,a1,0x2
    800031f6:	ecc40613          	addi	a2,s0,-308
    800031fa:	95b2                	add	a1,a1,a2
    800031fc:	418c                	lw	a1,0(a1)
    800031fe:	03000613          	li	a2,48
    80003202:	05700693          	li	a3,87
    80003206:	4725                	li	a4,9
    80003208:	eaa43c23          	sd	a0,-328(s0)
    8000320c:	eab43823          	sd	a1,-336(s0)
    80003210:	eac43423          	sd	a2,-344(s0)
    80003214:	ead43023          	sd	a3,-352(s0)
    80003218:	00b76663          	bltu	a4,a1,80003224 <printnum+0x106>
    8000321c:	ea843503          	ld	a0,-344(s0)
    80003220:	eaa43023          	sd	a0,-352(s0)
    80003224:	ea043503          	ld	a0,-352(s0)
    80003228:	fe043583          	ld	a1,-32(s0)
    8000322c:	eb043603          	ld	a2,-336(s0)
    80003230:	9d31                	addw	a0,a0,a2
    80003232:	eb843683          	ld	a3,-328(s0)
    80003236:	9682                	jalr	a3
    80003238:	f9fff06f          	j	800031d6 <printnum+0xb8>
    8000323c:	6456                	ld	s0,336(sp)
    8000323e:	60f6                	ld	ra,344(sp)
    80003240:	6135                	addi	sp,sp,352
    80003242:	8082                	ret

0000000080003244 <list_getSize>:
    80003244:	1101                	addi	sp,sp,-32
    80003246:	ec06                	sd	ra,24(sp)
    80003248:	e822                	sd	s0,16(sp)
    8000324a:	1000                	addi	s0,sp,32
    8000324c:	fea43423          	sd	a0,-24(s0)
    80003250:	fe843503          	ld	a0,-24(s0)
    80003254:	4108                	lw	a0,0(a0)
    80003256:	6442                	ld	s0,16(sp)
    80003258:	60e2                	ld	ra,24(sp)
    8000325a:	6105                	addi	sp,sp,32
    8000325c:	8082                	ret

000000008000325e <list_init>:
    8000325e:	1101                	addi	sp,sp,-32
    80003260:	ec06                	sd	ra,24(sp)
    80003262:	e822                	sd	s0,16(sp)
    80003264:	1000                	addi	s0,sp,32
    80003266:	fea43423          	sd	a0,-24(s0)
    8000326a:	fe843503          	ld	a0,-24(s0)
    8000326e:	4581                	li	a1,0
    80003270:	c10c                	sw	a1,0(a0)
    80003272:	fe843503          	ld	a0,-24(s0)
    80003276:	e50c                	sd	a1,8(a0)
    80003278:	6442                	ld	s0,16(sp)
    8000327a:	60e2                	ld	ra,24(sp)
    8000327c:	6105                	addi	sp,sp,32
    8000327e:	8082                	ret

0000000080003280 <list_push>:
    80003280:	7179                	addi	sp,sp,-48
    80003282:	f406                	sd	ra,40(sp)
    80003284:	f022                	sd	s0,32(sp)
    80003286:	1800                	addi	s0,sp,48
    80003288:	862e                	mv	a2,a1
    8000328a:	fea43423          	sd	a0,-24(s0)
    8000328e:	feb42223          	sw	a1,-28(s0)
    80003292:	00000517          	auipc	a0,0x0
    80003296:	4b650513          	addi	a0,a0,1206 # 80003748 <g_nodeFreeList>
    8000329a:	650c                	ld	a1,8(a0)
    8000329c:	fcb43c23          	sd	a1,-40(s0)
    800032a0:	650c                	ld	a1,8(a0)
    800032a2:	658c                	ld	a1,8(a1)
    800032a4:	e50c                	sd	a1,8(a0)
    800032a6:	fe843503          	ld	a0,-24(s0)
    800032aa:	6508                	ld	a0,8(a0)
    800032ac:	fd843583          	ld	a1,-40(s0)
    800032b0:	e588                	sd	a0,8(a1)
    800032b2:	fd843503          	ld	a0,-40(s0)
    800032b6:	fe843583          	ld	a1,-24(s0)
    800032ba:	e588                	sd	a0,8(a1)
    800032bc:	fe442503          	lw	a0,-28(s0)
    800032c0:	fe843583          	ld	a1,-24(s0)
    800032c4:	658c                	ld	a1,8(a1)
    800032c6:	c188                	sw	a0,0(a1)
    800032c8:	fe843503          	ld	a0,-24(s0)
    800032cc:	410c                	lw	a1,0(a0)
    800032ce:	0585                	addi	a1,a1,1
    800032d0:	c10c                	sw	a1,0(a0)
    800032d2:	7402                	ld	s0,32(sp)
    800032d4:	70a2                	ld	ra,40(sp)
    800032d6:	6145                	addi	sp,sp,48
    800032d8:	8082                	ret

00000000800032da <list_pop>:
    800032da:	7179                	addi	sp,sp,-48
    800032dc:	f406                	sd	ra,40(sp)
    800032de:	f022                	sd	s0,32(sp)
    800032e0:	1800                	addi	s0,sp,48
    800032e2:	fea43423          	sd	a0,-24(s0)
    800032e6:	fe843503          	ld	a0,-24(s0)
    800032ea:	6508                	ld	a0,8(a0)
    800032ec:	4108                	lw	a0,0(a0)
    800032ee:	fca42e23          	sw	a0,-36(s0)
    800032f2:	fe843503          	ld	a0,-24(s0)
    800032f6:	6508                	ld	a0,8(a0)
    800032f8:	fea43023          	sd	a0,-32(s0)
    800032fc:	fe843503          	ld	a0,-24(s0)
    80003300:	650c                	ld	a1,8(a0)
    80003302:	658c                	ld	a1,8(a1)
    80003304:	e50c                	sd	a1,8(a0)
    80003306:	00000517          	auipc	a0,0x0
    8000330a:	44250513          	addi	a0,a0,1090 # 80003748 <g_nodeFreeList>
    8000330e:	650c                	ld	a1,8(a0)
    80003310:	fe043603          	ld	a2,-32(s0)
    80003314:	e60c                	sd	a1,8(a2)
    80003316:	fe043583          	ld	a1,-32(s0)
    8000331a:	e50c                	sd	a1,8(a0)
    8000331c:	fe843503          	ld	a0,-24(s0)
    80003320:	410c                	lw	a1,0(a0)
    80003322:	15fd                	addi	a1,a1,-1
    80003324:	c10c                	sw	a1,0(a0)
    80003326:	fdc42503          	lw	a0,-36(s0)
    8000332a:	7402                	ld	s0,32(sp)
    8000332c:	70a2                	ld	ra,40(sp)
    8000332e:	6145                	addi	sp,sp,48
    80003330:	8082                	ret

0000000080003332 <list_clear>:
    80003332:	1101                	addi	sp,sp,-32
    80003334:	ec06                	sd	ra,24(sp)
    80003336:	e822                	sd	s0,16(sp)
    80003338:	1000                	addi	s0,sp,32
    8000333a:	fea43423          	sd	a0,-24(s0)
    8000333e:	0040006f          	j	80003342 <list_clear+0x10>
    80003342:	fe843503          	ld	a0,-24(s0)
    80003346:	effff0ef          	jal	ra,80003244 <list_getSize>
    8000334a:	4585                	li	a1,1
    8000334c:	00b54a63          	blt	a0,a1,80003360 <list_clear+0x2e>
    80003350:	0040006f          	j	80003354 <list_clear+0x22>
    80003354:	fe843503          	ld	a0,-24(s0)
    80003358:	f83ff0ef          	jal	ra,800032da <list_pop>
    8000335c:	fe7ff06f          	j	80003342 <list_clear+0x10>
    80003360:	6442                	ld	s0,16(sp)
    80003362:	60e2                	ld	ra,24(sp)
    80003364:	6105                	addi	sp,sp,32
    80003366:	8082                	ret

0000000080003368 <towers_init>:
    80003368:	7179                	addi	sp,sp,-48
    8000336a:	f406                	sd	ra,40(sp)
    8000336c:	f022                	sd	s0,32(sp)
    8000336e:	1800                	addi	s0,sp,48
    80003370:	862e                	mv	a2,a1
    80003372:	fea43423          	sd	a0,-24(s0)
    80003376:	feb42223          	sw	a1,-28(s0)
    8000337a:	fe442503          	lw	a0,-28(s0)
    8000337e:	fe843583          	ld	a1,-24(s0)
    80003382:	c188                	sw	a0,0(a1)
    80003384:	fe843503          	ld	a0,-24(s0)
    80003388:	4581                	li	a1,0
    8000338a:	c14c                	sw	a1,4(a0)
    8000338c:	fe843503          	ld	a0,-24(s0)
    80003390:	0521                	addi	a0,a0,8
    80003392:	fcc43c23          	sd	a2,-40(s0)
    80003396:	fcb43823          	sd	a1,-48(s0)
    8000339a:	ec5ff0ef          	jal	ra,8000325e <list_init>
    8000339e:	fe843503          	ld	a0,-24(s0)
    800033a2:	0561                	addi	a0,a0,24
    800033a4:	ebbff0ef          	jal	ra,8000325e <list_init>
    800033a8:	fe843503          	ld	a0,-24(s0)
    800033ac:	02850513          	addi	a0,a0,40
    800033b0:	eafff0ef          	jal	ra,8000325e <list_init>
    800033b4:	fd043503          	ld	a0,-48(s0)
    800033b8:	fea42023          	sw	a0,-32(s0)
    800033bc:	0040006f          	j	800033c0 <towers_init+0x58>
    800033c0:	fe042503          	lw	a0,-32(s0)
    800033c4:	fe442583          	lw	a1,-28(s0)
    800033c8:	02b55763          	bge	a0,a1,800033f6 <towers_init+0x8e>
    800033cc:	0040006f          	j	800033d0 <towers_init+0x68>
    800033d0:	fe843503          	ld	a0,-24(s0)
    800033d4:	0521                	addi	a0,a0,8
    800033d6:	fe442583          	lw	a1,-28(s0)
    800033da:	fe042603          	lw	a2,-32(s0)
    800033de:	9d91                	subw	a1,a1,a2
    800033e0:	ea1ff0ef          	jal	ra,80003280 <list_push>
    800033e4:	0040006f          	j	800033e8 <towers_init+0x80>
    800033e8:	fe042503          	lw	a0,-32(s0)
    800033ec:	0505                	addi	a0,a0,1
    800033ee:	fea42023          	sw	a0,-32(s0)
    800033f2:	fcfff06f          	j	800033c0 <towers_init+0x58>
    800033f6:	7402                	ld	s0,32(sp)
    800033f8:	70a2                	ld	ra,40(sp)
    800033fa:	6145                	addi	sp,sp,48
    800033fc:	8082                	ret

00000000800033fe <towers_clear>:
    800033fe:	1101                	addi	sp,sp,-32
    80003400:	ec06                	sd	ra,24(sp)
    80003402:	e822                	sd	s0,16(sp)
    80003404:	1000                	addi	s0,sp,32
    80003406:	fea43423          	sd	a0,-24(s0)
    8000340a:	fe843503          	ld	a0,-24(s0)
    8000340e:	0521                	addi	a0,a0,8
    80003410:	f23ff0ef          	jal	ra,80003332 <list_clear>
    80003414:	fe843503          	ld	a0,-24(s0)
    80003418:	0561                	addi	a0,a0,24
    8000341a:	f19ff0ef          	jal	ra,80003332 <list_clear>
    8000341e:	fe843503          	ld	a0,-24(s0)
    80003422:	02850513          	addi	a0,a0,40
    80003426:	f0dff0ef          	jal	ra,80003332 <list_clear>
    8000342a:	fe843503          	ld	a0,-24(s0)
    8000342e:	410c                	lw	a1,0(a0)
    80003430:	f39ff0ef          	jal	ra,80003368 <towers_init>
    80003434:	6442                	ld	s0,16(sp)
    80003436:	60e2                	ld	ra,24(sp)
    80003438:	6105                	addi	sp,sp,32
    8000343a:	8082                	ret

000000008000343c <towers_solve_h>:
    8000343c:	7139                	addi	sp,sp,-64
    8000343e:	fc06                	sd	ra,56(sp)
    80003440:	f822                	sd	s0,48(sp)
    80003442:	0080                	addi	s0,sp,64
    80003444:	87ae                	mv	a5,a1
    80003446:	fea43423          	sd	a0,-24(s0)
    8000344a:	feb42223          	sw	a1,-28(s0)
    8000344e:	fcc43c23          	sd	a2,-40(s0)
    80003452:	fcd43823          	sd	a3,-48(s0)
    80003456:	fce43423          	sd	a4,-56(s0)
    8000345a:	fe442503          	lw	a0,-28(s0)
    8000345e:	4585                	li	a1,1
    80003460:	02b51763          	bne	a0,a1,8000348e <towers_solve_h+0x52>
    80003464:	0040006f          	j	80003468 <towers_solve_h+0x2c>
    80003468:	fd843503          	ld	a0,-40(s0)
    8000346c:	e6fff0ef          	jal	ra,800032da <list_pop>
    80003470:	fca42223          	sw	a0,-60(s0)
    80003474:	fc843503          	ld	a0,-56(s0)
    80003478:	fc442583          	lw	a1,-60(s0)
    8000347c:	e05ff0ef          	jal	ra,80003280 <list_push>
    80003480:	fe843503          	ld	a0,-24(s0)
    80003484:	414c                	lw	a1,4(a0)
    80003486:	0585                	addi	a1,a1,1
    80003488:	c14c                	sw	a1,4(a0)
    8000348a:	0520006f          	j	800034dc <towers_solve_h+0xa0>
    8000348e:	fe843503          	ld	a0,-24(s0)
    80003492:	fe442583          	lw	a1,-28(s0)
    80003496:	fd843603          	ld	a2,-40(s0)
    8000349a:	fc843683          	ld	a3,-56(s0)
    8000349e:	fd043703          	ld	a4,-48(s0)
    800034a2:	35fd                	addiw	a1,a1,-1
    800034a4:	f99ff0ef          	jal	ra,8000343c <towers_solve_h>
    800034a8:	fe843503          	ld	a0,-24(s0)
    800034ac:	fd843603          	ld	a2,-40(s0)
    800034b0:	fd043683          	ld	a3,-48(s0)
    800034b4:	fc843703          	ld	a4,-56(s0)
    800034b8:	4585                	li	a1,1
    800034ba:	f83ff0ef          	jal	ra,8000343c <towers_solve_h>
    800034be:	fe843503          	ld	a0,-24(s0)
    800034c2:	fe442583          	lw	a1,-28(s0)
    800034c6:	fd043603          	ld	a2,-48(s0)
    800034ca:	fd843683          	ld	a3,-40(s0)
    800034ce:	fc843703          	ld	a4,-56(s0)
    800034d2:	35fd                	addiw	a1,a1,-1
    800034d4:	f69ff0ef          	jal	ra,8000343c <towers_solve_h>
    800034d8:	0040006f          	j	800034dc <towers_solve_h+0xa0>
    800034dc:	7442                	ld	s0,48(sp)
    800034de:	70e2                	ld	ra,56(sp)
    800034e0:	6121                	addi	sp,sp,64
    800034e2:	8082                	ret

00000000800034e4 <towers_solve>:
    800034e4:	1101                	addi	sp,sp,-32
    800034e6:	ec06                	sd	ra,24(sp)
    800034e8:	e822                	sd	s0,16(sp)
    800034ea:	1000                	addi	s0,sp,32
    800034ec:	fea43423          	sd	a0,-24(s0)
    800034f0:	fe843503          	ld	a0,-24(s0)
    800034f4:	410c                	lw	a1,0(a0)
    800034f6:	00850613          	addi	a2,a0,8
    800034fa:	01850693          	addi	a3,a0,24
    800034fe:	02850713          	addi	a4,a0,40
    80003502:	f3bff0ef          	jal	ra,8000343c <towers_solve_h>
    80003506:	6442                	ld	s0,16(sp)
    80003508:	60e2                	ld	ra,24(sp)
    8000350a:	6105                	addi	sp,sp,32
    8000350c:	8082                	ret

000000008000350e <towers_verify>:
    8000350e:	7139                	addi	sp,sp,-64
    80003510:	fc06                	sd	ra,56(sp)
    80003512:	f822                	sd	s0,48(sp)
    80003514:	0080                	addi	s0,sp,64
    80003516:	fea43023          	sd	a0,-32(s0)
    8000351a:	4501                	li	a0,0
    8000351c:	fca42a23          	sw	a0,-44(s0)
    80003520:	fe043583          	ld	a1,-32(s0)
    80003524:	05a1                	addi	a1,a1,8
    80003526:	fca43423          	sd	a0,-56(s0)
    8000352a:	852e                	mv	a0,a1
    8000352c:	d19ff0ef          	jal	ra,80003244 <list_getSize>
    80003530:	fc843583          	ld	a1,-56(s0)
    80003534:	00b50963          	beq	a0,a1,80003546 <towers_verify+0x38>
    80003538:	0040006f          	j	8000353c <towers_verify+0x2e>
    8000353c:	4509                	li	a0,2
    8000353e:	fea42623          	sw	a0,-20(s0)
    80003542:	0c80006f          	j	8000360a <towers_verify+0xfc>
    80003546:	fe043503          	ld	a0,-32(s0)
    8000354a:	0561                	addi	a0,a0,24
    8000354c:	cf9ff0ef          	jal	ra,80003244 <list_getSize>
    80003550:	4581                	li	a1,0
    80003552:	00b50963          	beq	a0,a1,80003564 <towers_verify+0x56>
    80003556:	0040006f          	j	8000355a <towers_verify+0x4c>
    8000355a:	450d                	li	a0,3
    8000355c:	fea42623          	sw	a0,-20(s0)
    80003560:	0aa0006f          	j	8000360a <towers_verify+0xfc>
    80003564:	fe043503          	ld	a0,-32(s0)
    80003568:	02850513          	addi	a0,a0,40
    8000356c:	cd9ff0ef          	jal	ra,80003244 <list_getSize>
    80003570:	fe043583          	ld	a1,-32(s0)
    80003574:	418c                	lw	a1,0(a1)
    80003576:	00b50963          	beq	a0,a1,80003588 <towers_verify+0x7a>
    8000357a:	0040006f          	j	8000357e <towers_verify+0x70>
    8000357e:	4511                	li	a0,4
    80003580:	fea42623          	sw	a0,-20(s0)
    80003584:	0860006f          	j	8000360a <towers_verify+0xfc>
    80003588:	fe043503          	ld	a0,-32(s0)
    8000358c:	7908                	ld	a0,48(a0)
    8000358e:	fca43c23          	sd	a0,-40(s0)
    80003592:	0040006f          	j	80003596 <towers_verify+0x88>
    80003596:	fd843503          	ld	a0,-40(s0)
    8000359a:	4581                	li	a1,0
    8000359c:	04b50063          	beq	a0,a1,800035dc <towers_verify+0xce>
    800035a0:	0040006f          	j	800035a4 <towers_verify+0x96>
    800035a4:	fd442503          	lw	a0,-44(s0)
    800035a8:	0505                	addi	a0,a0,1
    800035aa:	fca42a23          	sw	a0,-44(s0)
    800035ae:	fd843503          	ld	a0,-40(s0)
    800035b2:	4108                	lw	a0,0(a0)
    800035b4:	fd442583          	lw	a1,-44(s0)
    800035b8:	00b50963          	beq	a0,a1,800035ca <towers_verify+0xbc>
    800035bc:	0040006f          	j	800035c0 <towers_verify+0xb2>
    800035c0:	4515                	li	a0,5
    800035c2:	fea42623          	sw	a0,-20(s0)
    800035c6:	0440006f          	j	8000360a <towers_verify+0xfc>
    800035ca:	0040006f          	j	800035ce <towers_verify+0xc0>
    800035ce:	fd843503          	ld	a0,-40(s0)
    800035d2:	6508                	ld	a0,8(a0)
    800035d4:	fca43c23          	sd	a0,-40(s0)
    800035d8:	fbfff06f          	j	80003596 <towers_verify+0x88>
    800035dc:	fe043503          	ld	a0,-32(s0)
    800035e0:	414c                	lw	a1,4(a0)
    800035e2:	00056503          	lwu	a0,0(a0)
    800035e6:	4605                	li	a2,1
    800035e8:	00a6153b          	sllw	a0,a2,a0
    800035ec:	357d                	addiw	a0,a0,-1
    800035ee:	00a58963          	beq	a1,a0,80003600 <towers_verify+0xf2>
    800035f2:	0040006f          	j	800035f6 <towers_verify+0xe8>
    800035f6:	4519                	li	a0,6
    800035f8:	fea42623          	sw	a0,-20(s0)
    800035fc:	00e0006f          	j	8000360a <towers_verify+0xfc>
    80003600:	4501                	li	a0,0
    80003602:	fea42623          	sw	a0,-20(s0)
    80003606:	0040006f          	j	8000360a <towers_verify+0xfc>
    8000360a:	fec42503          	lw	a0,-20(s0)
    8000360e:	7442                	ld	s0,48(sp)
    80003610:	70e2                	ld	ra,56(sp)
    80003612:	6121                	addi	sp,sp,64
    80003614:	8082                	ret

0000000080003616 <main>:
    80003616:	7119                	addi	sp,sp,-128
    80003618:	fc86                	sd	ra,120(sp)
    8000361a:	f8a2                	sd	s0,112(sp)
    8000361c:	0100                	addi	s0,sp,128
    8000361e:	862a                	mv	a2,a0
    80003620:	4681                	li	a3,0
    80003622:	fed42623          	sw	a3,-20(s0)
    80003626:	fea42423          	sw	a0,-24(s0)
    8000362a:	feb43023          	sd	a1,-32(s0)
    8000362e:	00000517          	auipc	a0,0x0
    80003632:	11a50513          	addi	a0,a0,282 # 80003748 <g_nodeFreeList>
    80003636:	f8a43c23          	sd	a0,-104(s0)
    8000363a:	f8c43823          	sd	a2,-112(s0)
    8000363e:	f8d43423          	sd	a3,-120(s0)
    80003642:	c1dff0ef          	jal	ra,8000325e <list_init>
    80003646:	00000517          	auipc	a0,0x0
    8000364a:	11250513          	addi	a0,a0,274 # 80003758 <g_nodePool>
    8000364e:	f9843583          	ld	a1,-104(s0)
    80003652:	e588                	sd	a0,8(a1)
    80003654:	461d                	li	a2,7
    80003656:	c190                	sw	a2,0(a1)
    80003658:	f8843603          	ld	a2,-120(s0)
    8000365c:	f530                	sd	a2,104(a0)
    8000365e:	06300693          	li	a3,99
    80003662:	d134                	sw	a3,96(a0)
    80003664:	fac42223          	sw	a2,-92(s0)
    80003668:	0040006f          	j	8000366c <main+0x56>
    8000366c:	fa442503          	lw	a0,-92(s0)
    80003670:	4595                	li	a1,5
    80003672:	04a5c063          	blt	a1,a0,800036b2 <main+0x9c>
    80003676:	0040006f          	j	8000367a <main+0x64>
    8000367a:	fa442503          	lw	a0,-92(s0)
    8000367e:	0015059b          	addiw	a1,a0,1
    80003682:	0592                	slli	a1,a1,0x4
    80003684:	00000617          	auipc	a2,0x0
    80003688:	0d460613          	addi	a2,a2,212 # 80003758 <g_nodePool>
    8000368c:	95b2                	add	a1,a1,a2
    8000368e:	0512                	slli	a0,a0,0x4
    80003690:	9532                	add	a0,a0,a2
    80003692:	e50c                	sd	a1,8(a0)
    80003694:	fa442503          	lw	a0,-92(s0)
    80003698:	00451593          	slli	a1,a0,0x4
    8000369c:	95b2                	add	a1,a1,a2
    8000369e:	c188                	sw	a0,0(a1)
    800036a0:	0040006f          	j	800036a4 <main+0x8e>
    800036a4:	fa442503          	lw	a0,-92(s0)
    800036a8:	0505                	addi	a0,a0,1
    800036aa:	faa42223          	sw	a0,-92(s0)
    800036ae:	fbfff06f          	j	8000366c <main+0x56>
    800036b2:	fa840513          	addi	a0,s0,-88
    800036b6:	459d                	li	a1,7
    800036b8:	f8a43023          	sd	a0,-128(s0)
    800036bc:	cadff0ef          	jal	ra,80003368 <towers_init>
    800036c0:	f8043503          	ld	a0,-128(s0)
    800036c4:	d3bff0ef          	jal	ra,800033fe <towers_clear>
    800036c8:	4505                	li	a0,1
    800036ca:	937fe0ef          	jal	ra,80002000 <setStats>
    800036ce:	f8043503          	ld	a0,-128(s0)
    800036d2:	e13ff0ef          	jal	ra,800034e4 <towers_solve>
    800036d6:	4501                	li	a0,0
    800036d8:	929fe0ef          	jal	ra,80002000 <setStats>
    800036dc:	f8043503          	ld	a0,-128(s0)
    800036e0:	e2fff0ef          	jal	ra,8000350e <towers_verify>
    800036e4:	7446                	ld	s0,112(sp)
    800036e6:	70e6                	ld	ra,120(sp)
    800036e8:	6109                	addi	sp,sp,128
    800036ea:	8082                	ret

Disassembly of section .rodata.str1.1:

00000000800036ec <__global_pointer$-0x838>:
    800036ec:	636d                	lui	t1,0x1b
    800036ee:	6379                	lui	t1,0x1e
    800036f0:	656c                	ld	a1,200(a0)
    800036f2:	6d00                	ld	s0,24(a0)
    800036f4:	6e69                	lui	t3,0x1a
    800036f6:	65727473          	csrrci	s0,0x657,4
    800036fa:	0074                	addi	a3,sp,12
    800036fc:	6d49                	lui	s10,0x12
    800036fe:	6c70                	ld	a2,216(s0)
    80003700:	6d65                	lui	s10,0x19
    80003702:	6e65                	lui	t3,0x19
    80003704:	2074                	fld	fa3,192(s0)
    80003706:	616d                	addi	sp,sp,240
    80003708:	6e69                	lui	t3,0x1a
    8000370a:	2928                	fld	fa0,80(a0)
    8000370c:	202c                	fld	fa1,64(s0)
    8000370e:	6f66                	ld	t5,88(sp)
    80003710:	000a216f          	jal	sp,800a5710 <__global_pointer$+0xa17ec>
    80003714:	7325                	lui	t1,0xfffe9
    80003716:	3d20                	fld	fs0,120(a0)
    80003718:	2520                	fld	fs0,72(a0)
    8000371a:	0a64                	addi	s1,sp,284
    8000371c:	2800                	fld	fs0,16(s0)
    8000371e:	756e                	ld	a0,248(sp)
    80003720:	6c6c                	ld	a1,216(s0)
    80003722:	0029                	c.nop	10

Disassembly of section .bss:

0000000080003728 <counters>:
	...

0000000080003738 <counter_names>:
	...

0000000080003748 <g_nodeFreeList>:
	...

0000000080003758 <g_nodePool>:
	...

Disassembly of section .tbss:

0000000080003800 <putchar.buflen+0x800037c0>:
	...

Disassembly of section .comment:

0000000000000000 <.comment>:
   0:	6e616c63          	bltu	sp,t1,6f8 <putchar.buflen+0x6b8>
   4:	65762067          	0x65762067
   8:	7372                	ld	t1,312(sp)
   a:	6f69                	lui	t5,0x1a
   c:	206e                	fld	ft0,216(sp)
   e:	2e39                	addiw	t3,t3,14
  10:	2e30                	fld	fa2,88(a2)
  12:	2031                	0x2031
  14:	6728                	ld	a0,72(a4)
  16:	7469                	lui	s0,0xffffa
  18:	6740                	ld	s0,136(a4)
  1a:	7469                	lui	s0,0xffffa
  1c:	7568                	ld	a0,232(a0)
  1e:	2e62                	fld	ft8,24(sp)
  20:	3a6d6f63          	bltu	s10,t1,3de <putchar.buflen+0x39e>
  24:	5554                	lw	a3,44(a0)
  26:	5344                	lw	s1,36(a4)
  28:	692f4c53          	0x692f4c53
  2c:	746e                	ld	s0,248(sp)
  2e:	7265                	lui	tp,0xffff9
  30:	696d                	lui	s2,0x1b
  32:	7474                	ld	a3,232(s0)
  34:	6e65                	lui	t3,0x19
  36:	2d74                	fld	fa3,216(a0)
  38:	6972                	ld	s2,280(sp)
  3a:	762d6373          	csrrsi	t1,0x762,26
  3e:	672e                	ld	a4,200(sp)
  40:	7469                	lui	s0,0xffffa
  42:	6220                	ld	s0,64(a2)
  44:	38373663          	0x38373663
  48:	3935                	addiw	s2,s2,-19
  4a:	6232                	ld	tp,264(sp)
  4c:	6564                	ld	s1,200(a0)
  4e:	6166                	ld	sp,88(sp)
  50:	3835                	addiw	a6,a6,-19
  52:	6532                	ld	a0,264(sp)
  54:	3366                	fld	ft6,120(sp)
  56:	6331                	lui	t1,0xc
  58:	6139                	addi	sp,sp,448
  5a:	6536                	ld	a0,328(sp)
  5c:	32373533          	0x32373533
  60:	38396363          	bltu	s2,gp,3e6 <putchar.buflen+0x3a6>
  64:	3935                	addiw	s2,s2,-19
  66:	3731                	addiw	a4,a4,-20
  68:	3134                	fld	fa3,96(a0)
  6a:	          	lui	s2,0x2
