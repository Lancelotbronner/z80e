//
//  Utilities.swift
//  Tests/z80e
//
//  Created by Christophe Bronner on 2024-03-08.
//

import XCTest
import z80e

open class XCTestCaseAsic: XCTestCase {

	//MARK: - Device Management

	open var _type: ti_device_type { TI83p }

	public internal(set) var _asic: UnsafeMutablePointer<asic_t>!

	public var asic: asic_t {
		_read { yield _asic.pointee }
		_modify { yield &_asic.pointee }
	}

	public func flash(_ data: [UInt8]) {
		for i in data.indices {
			asic.mmu.flash[i] = data[i];
		}
	}

	public func read_byte(_ address: UInt16) -> UInt8 {
		cpu_read_byte(&asic.cpu, address)
	}

	public func write_byte(_ address: UInt16, _ value: UInt8) {
		cpu_write_byte(&asic.cpu, address, value)
	}

	public func read_word(_ address: UInt16) -> UInt16 {
		cpu_read_word(&asic.cpu, address)
	}

	public func write_word(_ address: UInt16, _ value: UInt16) {
		cpu_write_word(&asic.cpu, address, value)
	}

	public func get(device position: Int) -> device {
		cpu_device(&asic.cpu, UInt8(truncatingIfNeeded: position)).pointee
	}

	//MARK: - Test Device

	public var _value: UInt8 = 0

	//MARK: - Hooks

	override public func setUp() {
		guard let __asic = asic_init(_type, nil) else {
			XCTFail("Failed to initialize device")
			return
		}
		_asic = __asic

		// Configure the default test device
		cpu_device(&asic.cpu, 0x12)[0] = device(
			data: Unmanaged<XCTestCaseAsic>
				.passUnretained(self)
				.toOpaque(),
			read: test_read,
			write: test_write)
	}

	override public func tearDown() {
		asic_free(_asic)
	}

}

private func test_read(_ device: UnsafeMutablePointer<device>) -> UInt8 {
	let test = Unmanaged<XCTestCaseAsic>
		.fromOpaque(device.pointee.data!)
		.takeUnretainedValue()
	return test._value
}

private func test_write(_ device: UnsafeMutablePointer<device>, _ value: UInt8) {
	let test = Unmanaged<XCTestCaseAsic>
		.fromOpaque(device.pointee.data!)
		.takeUnretainedValue()
	test._value = value
}
