package com.itranswarp.learnjava;

/**
 * 用户不存在异常
 * 继承 BaseException，因此也是非受检异常，方法上不用写 throws
 */
public class UserNotFoundException extends BaseException {

	public UserNotFoundException() {
		super();
	}

	public UserNotFoundException(String message, Throwable cause) {
		super(message, cause);
	}

	public UserNotFoundException(String message) {
		super(message);
	}

	public UserNotFoundException(Throwable cause) {
		super(cause);
	}

}
