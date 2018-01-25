DEVICE_READ32(PFIFO)
{
	DEVICE_READ32_SWITCH() {
	case NV_PFIFO_RAMHT:
		result = 0x03000100; // = NV_PFIFO_RAMHT_SIZE_4K | NV_PFIFO_RAMHT_BASE_ADDRESS(NumberOfPaddingBytes >> 12) | NV_PFIFO_RAMHT_SEARCH_128
		break;
	case NV_PFIFO_RAMFC:
		result = 0x00890110; // = ? | NV_PFIFO_RAMFC_SIZE_2K | ?
		break;
	case NV_PFIFO_INTR_0:
		result = pfifo.pending_interrupts;
		break;
	case NV_PFIFO_INTR_EN_0:
		result = pfifo.enabled_interrupts;
		break;
	case NV_PFIFO_RUNOUT_STATUS:
		result = NV_PFIFO_RUNOUT_STATUS_LOW_MARK; /* low mark empty */
		break;
	case NV_PFIFO_CACHE1_PUSH0:
		result = pfifo.cache1.push_enabled;
		break;
	case NV_PFIFO_CACHE1_PUSH1:
		SET_MASK(result, NV_PFIFO_CACHE1_PUSH1_CHID, pfifo.cache1.channel_id);
		SET_MASK(result, NV_PFIFO_CACHE1_PUSH1_MODE, pfifo.cache1.mode);
		break;
	case NV_PFIFO_CACHE1_STATUS: {
		std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);

		if (pfifo.cache1.cache.empty()) {
			result |= NV_PFIFO_CACHE1_STATUS_LOW_MARK; /* low mark empty */
		}

	}	break;
	case NV_PFIFO_CACHE1_DMA_PUSH:
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_PUSH_ACCESS,
			pfifo.cache1.dma_push_enabled);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_PUSH_STATUS,
			pfifo.cache1.dma_push_suspended);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_PUSH_BUFFER, 1); /* buffer emoty */
		break;
	case NV_PFIFO_CACHE1_DMA_STATE:
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE,
			pfifo.cache1.method_nonincreasing);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_STATE_METHOD,
			pfifo.cache1.method >> 2);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_STATE_SUBCHANNEL,
			pfifo.cache1.subchannel);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT,
			pfifo.cache1.method_count);
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_STATE_ERROR,
			pfifo.cache1.error);
		break;
	case NV_PFIFO_CACHE1_DMA_INSTANCE:
		SET_MASK(result, NV_PFIFO_CACHE1_DMA_INSTANCE_ADDRESS_MASK,
			pfifo.cache1.dma_instance >> 4);
		break;
	case NV_PFIFO_CACHE1_DMA_PUT:
		result = user.channel_control[pfifo.cache1.channel_id].dma_put;
		break;
	case NV_PFIFO_CACHE1_DMA_GET:
		result = user.channel_control[pfifo.cache1.channel_id].dma_get;
		break;
	case NV_PFIFO_CACHE1_DMA_SUBROUTINE:
		result = pfifo.cache1.subroutine_return
			| pfifo.cache1.subroutine_active;
		break;
	case NV_PFIFO_CACHE1_PULL0: {
		std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);
		result = pfifo.cache1.pull_enabled;
	} break;
	case NV_PFIFO_CACHE1_ENGINE: {
		std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);
		for (int i = 0; i < NV2A_NUM_SUBCHANNELS; i++) {
			result |= pfifo.cache1.bound_engines[i] << (i * 2);
		}
		
	}	break;
	case NV_PFIFO_CACHE1_DMA_DCOUNT:
		result = pfifo.cache1.dcount;
		break;
	case NV_PFIFO_CACHE1_DMA_GET_JMP_SHADOW:
		result = pfifo.cache1.get_jmp_shadow;
		break;
	case NV_PFIFO_CACHE1_DMA_RSVD_SHADOW:
		result = pfifo.cache1.rsvd_shadow;
		break;
	case NV_PFIFO_CACHE1_DMA_DATA_SHADOW:
		result = pfifo.cache1.data_shadow;
		break;
	default:
		DEVICE_READ32_REG(pfifo); // Was : DEBUG_READ32_UNHANDLED(PFIFO);
		break;
	}

	DEVICE_READ32_END(PFIFO);
}

DEVICE_WRITE32(PFIFO)
{
	switch(addr) {
		case NV_PFIFO_INTR_0:
			pfifo.pending_interrupts &= ~value;
			update_irq();
			break;
		case NV_PFIFO_INTR_EN_0:
			pfifo.enabled_interrupts = value;
			update_irq();
			break;
		case NV_PFIFO_CACHE1_PUSH0:
			pfifo.cache1.push_enabled = value & NV_PFIFO_CACHE1_PUSH0_ACCESS;
			break;
		case NV_PFIFO_CACHE1_PUSH1:
			pfifo.cache1.channel_id = GET_MASK(value, NV_PFIFO_CACHE1_PUSH1_CHID);
			pfifo.cache1.mode = (FifoMode)GET_MASK(value, NV_PFIFO_CACHE1_PUSH1_MODE);
			assert(pfifo.cache1.channel_id < NV2A_NUM_CHANNELS);
			break;
		case NV_PFIFO_CACHE1_DMA_PUSH:
			pfifo.cache1.dma_push_enabled = GET_MASK(value, NV_PFIFO_CACHE1_DMA_PUSH_ACCESS);
			if (pfifo.cache1.dma_push_suspended	&& !GET_MASK(value, NV_PFIFO_CACHE1_DMA_PUSH_STATUS)) {
				pfifo.cache1.dma_push_suspended = false;
				pfifo_run_pusher();
			}
			pfifo.cache1.dma_push_suspended = GET_MASK(value, NV_PFIFO_CACHE1_DMA_PUSH_STATUS);
			break;
		case NV_PFIFO_CACHE1_DMA_STATE:
			pfifo.cache1.method_nonincreasing =	GET_MASK(value, NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE);
			pfifo.cache1.method = GET_MASK(value, NV_PFIFO_CACHE1_DMA_STATE_METHOD) << 2;
			pfifo.cache1.subchannel = GET_MASK(value, NV_PFIFO_CACHE1_DMA_STATE_SUBCHANNEL);
			pfifo.cache1.method_count =	GET_MASK(value, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT);
			pfifo.cache1.error = GET_MASK(value, NV_PFIFO_CACHE1_DMA_STATE_ERROR);
			break;
		case NV_PFIFO_CACHE1_DMA_INSTANCE:
			pfifo.cache1.dma_instance =	GET_MASK(value, NV_PFIFO_CACHE1_DMA_INSTANCE_ADDRESS_MASK) << 4;
			break;
		case NV_PFIFO_CACHE1_DMA_PUT:
			user.channel_control[pfifo.cache1.channel_id].dma_put = value;
			break;
		case NV_PFIFO_CACHE1_DMA_GET:
			user.channel_control[pfifo.cache1.channel_id].dma_get = value;
			break;
		case NV_PFIFO_CACHE1_DMA_SUBROUTINE:
			pfifo.cache1.subroutine_return = (value & NV_PFIFO_CACHE1_DMA_SUBROUTINE_RETURN_OFFSET);
			pfifo.cache1.subroutine_active = (value & NV_PFIFO_CACHE1_DMA_SUBROUTINE_STATE);
			break;
		case NV_PFIFO_CACHE1_PULL0: {
			std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);

			if ((value & NV_PFIFO_CACHE1_PULL0_ACCESS)
				&& !pfifo.cache1.pull_enabled) {
				pfifo.cache1.pull_enabled = true;

				/* the puller thread should wake up */
				pfifo.cache1.cache_cond.notify_all();
			} else if (!(value & NV_PFIFO_CACHE1_PULL0_ACCESS)
				&& pfifo.cache1.pull_enabled) {
				pfifo.cache1.pull_enabled = false;
			}
		}	break;
		case NV_PFIFO_CACHE1_ENGINE: {
			std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);

			for (int i = 0; i < NV2A_NUM_SUBCHANNELS; i++) {
				pfifo.cache1.bound_engines[i] = (FIFOEngine)((value >> (i * 2)) & 3);
			}

		} break;
		case NV_PFIFO_CACHE1_DMA_DCOUNT:
			pfifo.cache1.dcount = (value & NV_PFIFO_CACHE1_DMA_DCOUNT_VALUE);
			break;
		case NV_PFIFO_CACHE1_DMA_GET_JMP_SHADOW:
			pfifo.cache1.get_jmp_shadow = (value & NV_PFIFO_CACHE1_DMA_GET_JMP_SHADOW_OFFSET);
			break;
		case NV_PFIFO_CACHE1_DMA_RSVD_SHADOW:
			pfifo.cache1.rsvd_shadow = value;
			break;
		case NV_PFIFO_CACHE1_DMA_DATA_SHADOW:
			pfifo.cache1.data_shadow = value;
			break;
		default:
			DEVICE_WRITE32_REG(pfifo); // Was : DEBUG_WRITE32_UNHANDLED(PFIFO);
			break;
	}

	DEVICE_WRITE32_END(PFIFO);
}

/* pusher should be fine to run from a mimo handler
* whenever's it's convenient */
static void pfifo_run_pusher() {
	uint8_t channel_id;
	ChannelControl *control;
	Cache1State *state;
	CacheEntry *command;
	uint8_t *dma;
	xbaddr dma_len;
	uint32_t word;

	/* TODO: How is cache1 selected? */
	state = &pfifo.cache1;
	channel_id = state->channel_id;
	control = &user.channel_control[channel_id];

	if (!state->push_enabled) return;

	/* only handling DMA for now... */

	/* Channel running DMA */
	uint32_t channel_modes = pfifo.regs[NV_PFIFO_MODE];
	assert(channel_modes & (1 << channel_id));
	assert(state->mode == FIFO_DMA);

	if (!state->dma_push_enabled) return;
	if (state->dma_push_suspended) return;

	/* We're running so there should be no pending errors... */
	assert(state->error == NV_PFIFO_CACHE1_DMA_STATE_ERROR_NONE);

	dma = (uint8_t*)nv_dma_map(state->dma_instance, &dma_len);

	printf("DMA pusher: max 0x%08X, 0x%08X - 0x%08X\n",
		dma_len, control->dma_get, control->dma_put);

	/* based on the convenient pseudocode in envytools */
	while (control->dma_get != control->dma_put) {
		if (control->dma_get >= dma_len) {
			state->error = NV_PFIFO_CACHE1_DMA_STATE_ERROR_PROTECTION;
			break;
		}

		word = ldl_le_p((uint32_t*)(dma + control->dma_get));
		control->dma_get += 4;

		if (state->method_count) {
			/* data word of methods command */
			state->data_shadow = word;

			command = (CacheEntry*)calloc(1, sizeof(CacheEntry));
			command->method = state->method;
			command->subchannel = state->subchannel;
			command->nonincreasing = state->method_nonincreasing;
			command->parameter = word;

			std::lock_guard<std::mutex> lk(state->mutex);
			state->cache.push(command);
			state->cache_cond.notify_all();

			if (!state->method_nonincreasing) {
				state->method += 4;
			}

			state->method_count--;
			state->dcount++;
		} else {
			/* no command active - this is the first word of a new one */
			state->rsvd_shadow = word;
			/* match all forms */
			if ((word & 0xe0000003) == 0x20000000) {
				/* old jump */
				state->get_jmp_shadow = control->dma_get;
				control->dma_get = word & 0x1fffffff;
				printf("pb OLD_JMP 0x%08X\n", control->dma_get);
			}
			else if ((word & 3) == 1) {
				/* jump */
				state->get_jmp_shadow = control->dma_get;
				control->dma_get = word & 0xfffffffc;
				printf("pb JMP 0x%08X\n", control->dma_get);
			}
			else if ((word & 3) == 2) {
				/* call */
				if (state->subroutine_active) {
					state->error = NV_PFIFO_CACHE1_DMA_STATE_ERROR_CALL;
					break;
				}
				state->subroutine_return = control->dma_get;
				state->subroutine_active = true;
				control->dma_get = word & 0xfffffffc;
				printf("pb CALL 0x%08X\n", control->dma_get);
			}
			else if (word == 0x00020000) {
				/* return */
				if (!state->subroutine_active) {
					state->error = NV_PFIFO_CACHE1_DMA_STATE_ERROR_RETURN;
					break;
				}
				control->dma_get = state->subroutine_return;
				state->subroutine_active = false;
				printf("pb RET 0x%08X\n", control->dma_get);
			}
			else if ((word & 0xe0030003) == 0) {
				/* increasing methods */
				state->method = word & 0x1fff;
				state->subchannel = (word >> 13) & 7;
				state->method_count = (word >> 18) & 0x7ff;
				state->method_nonincreasing = false;
				state->dcount = 0;
			}
			else if ((word & 0xe0030003) == 0x40000000) {
				/* non-increasing methods */
				state->method = word & 0x1fff;
				state->subchannel = (word >> 13) & 7;
				state->method_count = (word >> 18) & 0x7ff;
				state->method_nonincreasing = true;
				state->dcount = 0;
			}
			else {
				printf("pb reserved cmd 0x%08X - 0x%08X\n",
					control->dma_get, word);
				state->error = NV_PFIFO_CACHE1_DMA_STATE_ERROR_RESERVED_CMD;
				break;
			}
		}
	}

	printf("DMA pusher done: max 0x%08X, 0x%08X - 0x%08X\n",
		dma_len, control->dma_get, control->dma_put);

	if (state->error) {
		printf("pb error: %d\n", state->error);
		assert(false);

		state->dma_push_suspended = true;

		pfifo.pending_interrupts |= NV_PFIFO_INTR_0_DMA_PUSHER;
		update_irq();
	}
}

static void* pfifo_puller_thread()
{
	Cache1State *state = &pfifo.cache1;

	while (true) {
		// Scope the lock so that it automatically unlocks at tne end of this block
		{
			std::unique_lock<std::mutex> lk(state->mutex);

			while (state->cache.empty() || !state->pull_enabled) {
				state->cache_cond.wait(lk);
			}

			// Copy cache to working_cache
			while (!state->cache.empty()) {
				state->working_cache.push(state->cache.front());
				state->cache.pop();
			}
		}

		while (!state->working_cache.empty()) {
			CacheEntry* command = state->working_cache.front();
			state->working_cache.pop();

			if (command->method == 0) {
				// qemu_mutex_lock_iothread();
				RAMHTEntry entry = ramht_lookup(command->parameter);
				assert(entry.valid);

				assert(entry.channel_id == state->channel_id);
				// qemu_mutex_unlock_iothread();

				switch (entry.engine) {
				case ENGINE_GRAPHICS:
					pgraph_context_switch(entry.channel_id);
					pgraph_wait_fifo_access();
					pgraph_method(command->subchannel, 0, entry.instance);
					break;
				default:
					assert(false);
					break;
				}

				/* the engine is bound to the subchannel */
				std::lock_guard<std::mutex> lk(pfifo.cache1.mutex);
				state->bound_engines[command->subchannel] = entry.engine;
				state->last_engine = entry.engine;
			} else if (command->method >= 0x100) {
				/* method passed to engine */

				uint32_t parameter = command->parameter;

				/* methods that take objects.
				* TODO: Check this range is correct for the nv2a */
				if (command->method >= 0x180 && command->method < 0x200) {
					//qemu_mutex_lock_iothread();
					RAMHTEntry entry = ramht_lookup(parameter);
					assert(entry.valid);
					assert(entry.channel_id == state->channel_id);
					parameter = entry.instance;
					//qemu_mutex_unlock_iothread();
				}

				// qemu_mutex_lock(&state->cache_lock);
				enum FIFOEngine engine = state->bound_engines[command->subchannel];
				// qemu_mutex_unlock(&state->cache_lock);

				switch (engine) {
				case ENGINE_GRAPHICS:
					pgraph_wait_fifo_access();
					pgraph_method(command->subchannel, command->method, parameter);
					break;
				default:
					assert(false);
					break;
				}

				// qemu_mutex_lock(&state->cache_lock);
				state->last_engine = state->bound_engines[command->subchannel];
				// qemu_mutex_unlock(&state->cache_lock);
			}

			free(command);
		}
	}

	return NULL;
}

static uint32_t ramht_hash(uint32_t handle)
{
	unsigned int ramht_size = 1 << (GET_MASK(pfifo.regs[NV_PFIFO_RAMHT], NV_PFIFO_RAMHT_SIZE_MASK) + 12);

	/* XXX: Think this is different to what nouveau calculates... */
	unsigned int bits = ffs(ramht_size) - 2;

	uint32_t hash = 0;
	while (handle) {
		hash ^= (handle & ((1 << bits) - 1));
		handle >>= bits;
	}
	hash ^= pfifo.cache1.channel_id << (bits - 4);

	return hash;
}


static RAMHTEntry ramht_lookup(uint32_t handle)
{
	unsigned int ramht_size = 1 << (GET_MASK(pfifo.regs[NV_PFIFO_RAMHT], NV_PFIFO_RAMHT_SIZE_MASK) + 12);

	uint32_t hash = ramht_hash(handle);
	assert(hash * 8 < ramht_size);

	uint32_t ramht_address =
		GET_MASK(pfifo.regs[NV_PFIFO_RAMHT],
			NV_PFIFO_RAMHT_BASE_ADDRESS_MASK) << 12;

	uint8_t *entry_ptr = (uint8_t*)(NV2A_ADDR + NV_PRAMIN_ADDR + ramht_address + hash * 8);

	uint32_t entry_handle = ldl_le_p((uint32_t*)entry_ptr);
	uint32_t entry_context = ldl_le_p((uint32_t*)(entry_ptr + 4));

	RAMHTEntry entry;
	entry.handle = entry_handle;
	entry.instance = (entry_context & NV_RAMHT_INSTANCE) << 4;
	entry.engine = (FIFOEngine)((entry_context & NV_RAMHT_ENGINE) >> 16);
	entry.channel_id = (entry_context & NV_RAMHT_CHID) >> 24;
	entry.valid = entry_context & NV_RAMHT_STATUS;

	return entry;
}

