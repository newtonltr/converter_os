#include "thread_init.h"
#include "tx_thread.h"
#include "eth.h"
#include "nx_stm32_eth_driver.h"

// ---------thread parameters
#define THREAD_INIT_STACK_SIZE		4096u
#define THREAD_INIT_PRIO			28u
TX_THREAD thread_init_block;
uint64_t thread_init_stack[THREAD_INIT_STACK_SIZE/8];
void thread_init(UINT input);

// ---------netxduo parameters
NX_PACKET_POOL    pool_0;
NX_IP             ip_0;
#define NX_PACKET_POOL_SIZE ((1536 + sizeof(NX_PACKET)) * 8)
ULONG  packet_pool_area[NX_PACKET_POOL_SIZE/4 + 4] __attribute__((section(".NetXPoolSection")));
ULONG  arp_space_area[52*20 / sizeof(ULONG)] __attribute__((section(".NetXPoolSection")));

#define IP_ADDR0                        192
#define IP_ADDR1                        168
#define IP_ADDR2                        0
#define IP_ADDR3                        135

#define  THREAD_NETX_IP0_PRIO0                          2u
#define  THREAD_NETX_IP0_STK_SIZE                     	1024*16u
static   uint64_t  thread_netx_ip0_stack[THREAD_NETX_IP0_STK_SIZE/8];

// ---------
void  tx_application_define(void *first_unused_memory)
{
	tx_thread_create(&thread_init_block, 
					"init", 
					thread_init, 
					0, 
					&thread_init_stack[0],
					THREAD_INIT_STACK_SIZE, 
					THREAD_INIT_PRIO, 
					THREAD_INIT_PRIO, 
					TX_NO_TIME_SLICE, 
					TX_AUTO_START);

}

uint32_t thread_init_heartbeat = 0;

void thread_init(UINT input)
{
  // ---------netxduo init
	UINT status;

	HAL_ETH_DeInit(&heth);
	nx_system_initialize();
	status = nx_packet_pool_create(&pool_0,
									"NetX Main Packet Pool",
									1536,  (ULONG*)(((int)packet_pool_area + 15) & ~15) ,
									NX_PACKET_POOL_SIZE);
	status = nx_ip_create(&ip_0,
						"NetX IP0",
						IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3),
						0xFFFFFF00UL,
						&pool_0, nx_stm32_eth_driver,
						(UCHAR*)thread_netx_ip0_stack,
						sizeof(thread_netx_ip0_stack),
						THREAD_NETX_IP0_PRIO0);
	status = nx_arp_enable(&ip_0, (void *)arp_space_area, sizeof(arp_space_area));
	status = nx_ip_fragment_enable(&ip_0);
	status = nx_tcp_enable(&ip_0);
	status = nx_udp_enable(&ip_0);
	status = nx_icmp_enable(&ip_0);

	ULONG gateway_ip = IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
	gateway_ip = (gateway_ip & 0xFFFFFF00) | 0x01;
	nx_ip_gateway_address_set(&ip_0, gateway_ip);

	while (1)
	{
		thread_init_heartbeat ++;
		sleep_ms(1000);
	}
	
}

