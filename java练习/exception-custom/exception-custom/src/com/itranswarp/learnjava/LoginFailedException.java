package com.itranswarp.learnjava;

/**
 * 登录失败异常（用户名正确但密码错误）
 * 继承 BaseException，因此也是非受检异常，方法上不用写 throws
 */
public class LoginFailedException extends BaseException {

	public LoginFailedException() {
		super();
	}

	public LoginFailedException(String message, Throwable cause) {
		super(message, cause);
	}

	public LoginFailedException(String message) {
		super(message);
	}

	public LoginFailedException(Throwable cause) {
		super(cause);
	}

}
